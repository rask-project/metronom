#include "TimingAnalyzer.h"

#include "AudioDevices.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QCoreApplication>
#include <QMediaDevices>
#include <QPermissions>
#include <QSettings>
#include <QVariant>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace
{
constexpr int kProcessIntervalMs = 10;
constexpr double kClockWarmupSeconds = 0.25;
constexpr double kClockSmoothing = 0.02;

constexpr int kBlocksPerSecond = 400;
constexpr double kRefractorySeconds = 0.03;
// Opening the microphone makes a pop.
constexpr double kInputWarmupSeconds = 0.25;
// Instruments attack in 10-30 ms, so a block is compared with the background level, not with the previous block.
constexpr double kPreEmphasis = 0.95;
constexpr double kBackgroundSmoothing = 2.5 / 150.0; // per block: ~150 ms
constexpr double kOnsetRatio = 3.0;
constexpr double kRearmRatio = 2.0;
constexpr double kAttackStartRatio = 1.5;
constexpr double kMinOnsetEnergy = 1e-5; // -50 dBFS: quieter sounds around are not hits

// Calibration plays accented clicks at kCalibrationBpm (see Metronome); the first ones let the clocks settle.
constexpr qint64 kCalibrationSkipTicks = 2;
constexpr qint64 kCalibrationTicks = 10;
constexpr std::size_t kMinCalibrationSamples = 5;
constexpr double kMaxLatencySeconds = 0.4;

constexpr std::size_t kTickHistory = 32;
constexpr double kOnTimeSeconds = 0.025;
// The microphone hears the metronome's own clicks within a couple of ms of them (after the calibrated latency).
constexpr double kClickEchoSeconds = 0.008;
// Detections still arrive this long after the click window closes.
constexpr double kCloseMarginSeconds = 0.05;
// Hearing something right on most recent clicks means the microphone hears the metronome (no headphones).
constexpr int kEchoClicks = 6;
constexpr std::size_t kBpmIntervals = 8;
constexpr std::size_t kMinBpmIntervals = 2;
// With no hit for this many beats (and at least kMinHitGapSeconds) the result is cleared.
constexpr double kHitGapBeats = 2.0;
constexpr double kMinHitGapSeconds = 1.5;

double steadySeconds() noexcept
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

template <typename T> float toNormalized(T sample) noexcept
{
    if constexpr (std::is_same_v<T, float>) {
        return sample;
    }
    return static_cast<float>(sample) / 32768.0f;
}

QAudioFormat chooseInputFormat(const QAudioDevice &device)
{
    QAudioFormat format = device.preferredFormat();
    if (format.sampleFormat() == QAudioFormat::Float || format.sampleFormat() == QAudioFormat::Int16) {
        return format;
    }

    format.setSampleFormat(QAudioFormat::Float);
    if (device.isFormatSupported(format)) {
        return format;
    }
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}

double median(std::vector<double> values)
{
    const auto middle = values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
    std::ranges::nth_element(values, middle);
    return *middle;
}
} // namespace

void StreamClock::reset(int sampleRate) noexcept
{
    m_sampleRate = sampleRate;
    m_estimate = 0.0;
    m_valid.store(false, std::memory_order_relaxed);
}

void StreamClock::update(qint64 frame) noexcept
{
    const double frameSeconds = static_cast<double>(frame) / m_sampleRate;
    if (frameSeconds < kClockWarmupSeconds) {
        return;
    }

    const double offset = steadySeconds() - frameSeconds;
    m_estimate =
        m_valid.load(std::memory_order_relaxed) ? m_estimate + ((offset - m_estimate) * kClockSmoothing) : offset;
    m_offset.store(m_estimate, std::memory_order_relaxed);
    m_valid.store(true, std::memory_order_release);
}

std::optional<double> StreamClock::timeOf(qint64 frame) const noexcept
{
    if (!m_valid.load(std::memory_order_acquire)) {
        return std::nullopt;
    }
    return m_offset.load(std::memory_order_relaxed) + (static_cast<double>(frame) / m_sampleRate);
}

TimingAnalyzer::TimingAnalyzer(QObject *parent)
    : QObject(parent)
    , m_inputDevices(QMediaDevices::audioInputs())
    , m_currentInputDevice(
          AudioDevices::chosenOrDefaultIndex(m_inputDevices, QSettings().value("timing/input").toByteArray())
      )
{
    m_enabled = QSettings().value("timing/enabled", false).toBool();

    m_processTimer.setInterval(kProcessIntervalMs);
    connect(&m_processTimer, &QTimer::timeout, this, &TimingAnalyzer::process);
}

TimingAnalyzer::~TimingAnalyzer()
{
    stop();
}

bool TimingAnalyzer::enabled() const
{
    return m_enabled;
}

void TimingAnalyzer::setEnabled(bool enabled)
{
    if (enabled == m_enabled) {
        return;
    }

    m_enabled = enabled;
    QSettings().setValue("timing/enabled", enabled);
    if (enabled) {
        requestMicrophone();
    }
    emit enabledChanged();
}

bool TimingAnalyzer::calibrated() const
{
    return m_calibrated;
}

bool TimingAnalyzer::needsCalibration() const
{
    return m_enabled && !m_calibrated;
}

void TimingAnalyzer::setLatencyAdjustment(double seconds)
{
    m_latencyAdjustment = seconds;
}

QList<QString> TimingAnalyzer::inputDeviceNames() const
{
    return AudioDevices::names(m_inputDevices);
}

int TimingAnalyzer::currentInputDevice() const
{
    return m_currentInputDevice;
}

void TimingAnalyzer::setCurrentInputDevice(int index)
{
    if (index < 0 || index >= m_inputDevices.size() || index == m_currentInputDevice) {
        return;
    }

    m_currentInputDevice = index;
    QSettings().setValue("timing/input", m_inputDevices.value(index).id());
    loadCalibration();
    emit currentInputDeviceChanged();
    emit inputDeviceSwitched();
}

void TimingAnalyzer::refreshInputDevices()
{
    const QByteArray previousId = m_inputDevices.value(m_currentInputDevice).id();
    m_inputDevices = QMediaDevices::audioInputs();
    m_currentInputDevice =
        AudioDevices::chosenOrDefaultIndex(m_inputDevices, QSettings().value("timing/input").toByteArray());
    emit inputDevicesChanged();
    emit currentInputDeviceChanged();

    if (m_inputDevices.value(m_currentInputDevice).id() == previousId) {
        return;
    }
    loadCalibration();
    emit inputDeviceSwitched();
}

void TimingAnalyzer::setOutputDevice(const QByteArray &id)
{
    m_outputId = id;
    loadCalibration();
}

QString TimingAnalyzer::calibrationKey() const
{
    const QByteArray pair = m_outputId + '\n' + m_inputDevices.value(m_currentInputDevice).id();
    return QStringLiteral("timing/latency/") +
           QString::fromLatin1(pair.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void TimingAnalyzer::loadCalibration()
{
    const QVariant latency = QSettings().value(calibrationKey());
    m_calibrated = latency.isValid();
    m_latency = latency.toDouble();
    emit calibratedChanged();
}

TimingAnalyzer::Status TimingAnalyzer::status() const
{
    return m_status;
}

bool TimingAnalyzer::hasHit() const
{
    return m_hasHit;
}

int TimingAnalyzer::lastOffset() const
{
    return m_lastOffset;
}

TimingAnalyzer::Accuracy TimingAnalyzer::lastAccuracy() const
{
    return m_lastAccuracy;
}

int TimingAnalyzer::playedBpm() const
{
    return m_playedBpm;
}

QList<int> TimingAnalyzer::measureHits() const
{
    return m_measureHits;
}

void TimingAnalyzer::recalibrate()
{
    if (!m_calibrated) {
        return;
    }

    QSettings().remove(calibrationKey());
    m_calibrated = false;
    emit calibratedChanged();
}

void TimingAnalyzer::requestMicrophone()
{
    const QMicrophonePermission permission;
    if (qApp->checkPermission(permission) != Qt::PermissionStatus::Undetermined) {
        return;
    }
    qApp->requestPermission(permission, this, [this](const QPermission &result) {
        if (result.status() == Qt::PermissionStatus::Granted) {
            emit microphoneGranted();
        }
    });
}

bool TimingAnalyzer::start(int outputSampleRate, bool calibrate)
{
    if (!m_enabled) {
        return false;
    }
    if (!m_calibrated && !calibrate) {
        setStatus(CalibrationFailed);
        return false;
    }

    resetResults();
    m_tickQueue.clear();
    m_onsetQueue.clear();
    m_outputClock.reset(outputSampleRate);

    requestMicrophone();
    if (qApp->checkPermission(QMicrophonePermission{}) != Qt::PermissionStatus::Granted || !startSource()) {
        setStatus(MicrophoneUnavailable);
        return false;
    }

    m_active.store(true, std::memory_order_release);
    m_processTimer.start();
    setStatus(m_calibrated ? Listening : Calibrating);
    return m_status == Calibrating;
}

bool TimingAnalyzer::startSource()
{
    const QAudioDevice device =
        m_inputDevices.isEmpty() ? QMediaDevices::defaultAudioInput() : m_inputDevices.value(m_currentInputDevice);
    if (device.isNull()) {
        return false;
    }

    m_inputFormat = chooseInputFormat(device);
    m_inputChannels = std::max(1, m_inputFormat.channelCount());
    m_blockFrames = std::max(1, m_inputFormat.sampleRate() / kBlocksPerSecond);
    m_refractoryBlocks = static_cast<int>(std::lround(kRefractorySeconds * kBlocksPerSecond));
    m_blockEnergy = 0.0;
    m_background = 0.0;
    m_previousSample = 0.0f;
    m_recentEnergy.fill(0.0);
    m_recentIndex = 0;
    m_armed = true;
    m_inputFrames = 0;
    m_warmupFrames = std::lround(kInputWarmupSeconds * m_inputFormat.sampleRate());
    m_blockFill = 0;
    m_blocksSinceOnset = m_refractoryBlocks;
    m_inputClock.reset(m_inputFormat.sampleRate());

    m_source = std::make_unique<QAudioSource>(device, m_inputFormat);
    if (m_inputFormat.sampleFormat() == QAudioFormat::Int16) {
        m_source->start([this](QSpan<const qint16> samples) { processInput(samples); });
    } else {
        m_source->start([this](QSpan<const float> samples) { processInput(samples); });
    }

    if (m_source->error() == QtAudio::NoError) {
        return true;
    }
    m_source.reset();
    return false;
}

void TimingAnalyzer::stop()
{
    m_active.store(false, std::memory_order_release);
    m_processTimer.stop();
    if (m_source) {
        m_source->stop();
        m_source.reset();
    }
    setStatus(Idle);
}

void TimingAnalyzer::setStatus(Status status)
{
    if (status == m_status) {
        return;
    }
    m_status = status;
    emit statusChanged();
}

void TimingAnalyzer::resetResults()
{
    m_tickCount = 0;
    m_measure = 0;
    m_ticks.clear();
    m_pendingOnsets.clear();
    m_openTick.reset();
    m_lastResolvedTick = -1;
    m_echoes = 0xFF;
    m_calibrationLatencies.clear();
    clearResult();
}

void TimingAnalyzer::clearResult()
{
    m_measureHits.clear();
    m_hitsMeasure = -1;
    m_lastHitTime.reset();
    m_intervals.clear();
    if (m_hasHit) {
        m_hasHit = false;
        emit hit();
    }
    if (m_playedBpm != 0) {
        m_playedBpm = 0;
        emit playedBpmChanged();
    }
}

void TimingAnalyzer::outputCallback(qint64 framesWritten) noexcept
{
    if (m_active.load(std::memory_order_acquire)) {
        m_outputClock.update(framesWritten);
    }
}

void TimingAnalyzer::tickPlayed(const Tick &tick) noexcept
{
    if (m_active.load(std::memory_order_acquire)) {
        m_tickQueue.push(
            {.frame = tick.frame,
             .index = 0,
             .measure = 0,
             .bpm = static_cast<std::uint16_t>(tick.bpm),
             .beatsPerMeasure = static_cast<std::uint8_t>(tick.beatsPerMeasure),
             .subdivisions = static_cast<std::uint8_t>(tick.subdivisions),
             .beat = static_cast<std::uint8_t>(tick.beat),
             .subdivision = static_cast<std::uint8_t>(tick.subdivision)}
        );
    }
}

template <typename T> void TimingAnalyzer::processInput(QSpan<const T> samples) noexcept
{
    const qint64 frameCount = samples.size() / m_inputChannels;

    for (qint64 frame = 0; frame < frameCount; ++frame) {
        float mono = 0.0f;
        for (const T sample : samples.subspan(frame * m_inputChannels, m_inputChannels)) {
            mono += toNormalized(sample);
        }
        mono /= static_cast<float>(m_inputChannels);
        // Pre-emphasis: the attack's high frequencies count, the low sustain of the previous note much less.
        const double emphasized = mono - (kPreEmphasis * m_previousSample);
        m_previousSample = mono;
        m_blockEnergy += emphasized * emphasized;

        if (++m_blockFill == m_blockFrames) {
            finishBlock(m_inputFrames + frame + 1 - m_blockFrames);
        }
    }

    m_inputFrames += frameCount;
    m_inputClock.update(m_inputFrames);
}

// An onset is a block well above the background level; it starts where the attack left the background, a few
// blocks back for a slow attack. Another onset needs the level to fall back first.
void TimingAnalyzer::finishBlock(qint64 blockStart) noexcept
{
    const double energy = m_blockEnergy / m_blockFrames;
    m_blockEnergy = 0.0;
    m_blockFill = 0;

    const bool onset = m_armed && blockStart >= m_warmupFrames && m_blocksSinceOnset >= m_refractoryBlocks &&
                       energy > kMinOnsetEnergy && energy > kOnsetRatio * m_background;
    if (onset) {
        m_onsetQueue.push(blockStart - (attackBlocks() * m_blockFrames));
        m_armed = false;
        m_blocksSinceOnset = 0;
    }
    m_armed = m_armed || energy < kRearmRatio * m_background;

    m_recentIndex = (m_recentIndex + 1) % m_recentEnergy.size();
    m_recentEnergy[m_recentIndex] = energy;
    m_background += (energy - m_background) * kBackgroundSmoothing;
    m_blocksSinceOnset = std::min(m_blocksSinceOnset + 1, m_refractoryBlocks);
}

// The blocks before this one that were already above the background: the attack began there.
qint64 TimingAnalyzer::attackBlocks() const noexcept
{
    const std::size_t size = m_recentEnergy.size();
    qint64 blocks = 0;
    for (std::size_t back = 0; back < size; ++back) {
        if (m_recentEnergy[(m_recentIndex + size - back) % size] <= kAttackStartRatio * m_background) {
            break;
        }
        ++blocks;
    }
    return blocks;
}

void TimingAnalyzer::process()
{
    drainEvents();

    if (m_status == Calibrating) {
        processCalibration();
        return;
    }
    if (m_status == Listening) {
        matchOnsets();
        clearStaleHit();
        return;
    }
    m_pendingOnsets.clear();
}

void TimingAnalyzer::drainEvents()
{
    while (auto tick = m_tickQueue.pop()) {
        tick->index = m_tickCount++;
        if (tick->beat == 0 && tick->subdivision == 0) {
            ++m_measure;
        }
        tick->measure = m_measure;
        m_ticks.emplace_back(*tick);
    }
    while (m_ticks.size() > kTickHistory) {
        m_ticks.pop_front();
    }

    while (const auto onset = m_onsetQueue.pop()) {
        m_pendingOnsets.emplace_back(*onset);
    }
}

void TimingAnalyzer::processCalibration()
{
    while (!m_pendingOnsets.empty()) {
        const auto time = m_inputClock.timeOf(m_pendingOnsets.front());
        if (!time) {
            return;
        }
        m_pendingOnsets.pop_front();

        const auto tickTime = latestTickBefore(*time);
        if (!tickTime || *time - *tickTime > kMaxLatencySeconds) {
            continue;
        }
        m_calibrationLatencies.emplace_back(*time - *tickTime);
    }

    if (m_tickCount > kCalibrationTicks) {
        finishCalibration();
    }
}

void TimingAnalyzer::finishCalibration()
{
    m_pendingOnsets.clear();

    if (m_calibrationLatencies.size() < kMinCalibrationSamples) {
        setStatus(CalibrationFailed);
        emit calibrationFinished();
        return;
    }

    m_latency = median(m_calibrationLatencies) + m_latencyAdjustment;
    QSettings().setValue(calibrationKey(), m_latency);
    m_lastResolvedTick = m_tickCount - 1;
    m_calibrated = true;
    emit calibratedChanged();

    setStatus(Listening);
    emit calibrationFinished();
}

std::optional<double> TimingAnalyzer::latestTickBefore(double time) const
{
    std::optional<double> latest;
    for (const TickEvent &tick :
         m_ticks | std::views::filter([](const TickEvent &tick) { return tick.index >= kCalibrationSkipTicks; })) {
        const auto tickTime = m_outputClock.timeOf(tick.frame);
        if (tickTime && *tickTime <= time) {
            latest = tickTime;
        }
    }
    return latest;
}

void TimingAnalyzer::matchOnsets()
{
    while (!m_pendingOnsets.empty()) {
        const auto inputTime = m_inputClock.timeOf(m_pendingOnsets.front());
        if (!inputTime) {
            return;
        }

        const double time = *inputTime - m_latency;
        const auto match = nearestTick(time);
        if (!match) {
            return;
        }

        m_pendingOnsets.pop_front();
        collectHit({.match = *match, .time = time});
    }

    if (m_openTick && steadySeconds() - m_latency > m_openTick->closesAt + kCloseMarginSeconds) {
        resolveOpenTick();
    }
}

// Waits (nullopt) while the hit may belong to a click not scheduled yet: it is past the middle of the last interval.
std::optional<TimingAnalyzer::TickMatch> TimingAnalyzer::nearestTick(double time) const
{
    std::optional<TickMatch> best;
    bool reachedHit = false;

    for (const TickEvent &tick : m_ticks) {
        const auto tickTime = m_outputClock.timeOf(tick.frame);
        if (!tickTime) {
            continue;
        }

        reachedHit = reachedHit || *tickTime >= time;
        const double offset = time - *tickTime;
        if (best && std::abs(best->offset) <= std::abs(offset)) {
            continue;
        }
        best = TickMatch{
            .offset = offset,
            .tickTime = *tickTime,
            .tickSeconds = 60.0 / (tick.bpm * std::max<int>(1, tick.subdivisions)),
            .tick = tick
        };
    }

    if (reachedHit || (best && best->offset <= best->tickSeconds / 2.0)) {
        return best;
    }
    return std::nullopt;
}

double TimingAnalyzer::maxHitGap() const
{
    if (m_ticks.empty()) {
        return kMinHitGapSeconds;
    }
    return std::max(kMinHitGapSeconds, kHitGapBeats * 60.0 / m_ticks.back().bpm);
}

// The hits are compared on the steady clock, minus the round trip latency; a hit is only known once its click's
// window has closed.
void TimingAnalyzer::clearStaleHit()
{
    if (!m_lastHitTime || m_ticks.empty()) {
        return;
    }

    const TickEvent &tick = m_ticks.back();
    const double window = 30.0 / (tick.bpm * std::max<int>(1, tick.subdivisions));
    if (steadySeconds() - m_latency - *m_lastHitTime <= maxHitGap() + window + kCloseMarginSeconds) {
        return;
    }

    clearResult();
}

void TimingAnalyzer::collectHit(const Hit &hit)
{
    if (m_openTick && m_openTick->index != hit.match.tick.index) {
        resolveOpenTick();
    }
    if (!m_openTick) {
        m_openTick = OpenTick{
            .nearClick = std::nullopt,
            .apart = std::nullopt,
            .closesAt = hit.match.tickTime + (hit.match.tickSeconds / 2.0),
            .index = hit.match.tick.index,
            .shown = false
        };
    }
    if (m_openTick->shown) {
        return;
    }

    if (std::abs(hit.match.offset) > kClickEchoSeconds) {
        m_openTick->apart = hit;
    } else {
        m_openTick->nearClick = hit;
        if (std::popcount(m_echoes) >= kEchoClicks) {
            return;
        }
    }
    m_openTick->shown = true;
    registerHit(hit);
}

// A hit right on the click is the player too, unless the microphone keeps hearing the metronome's own clicks; a hit
// apart hides the echo, so only the clicks without one tell.
void TimingAnalyzer::resolveOpenTick()
{
    const std::optional<OpenTick> closed = std::exchange(m_openTick, std::nullopt);
    if (!closed) {
        return;
    }
    const OpenTick &open = *closed;
    recordSilentClicks(open.index);
    if (open.apart) {
        return;
    }

    m_echoes = static_cast<std::uint8_t>((m_echoes << 1U) | 1U);
    if (open.nearClick && !open.shown && std::popcount(m_echoes) < kEchoClicks) {
        registerHit(*open.nearClick);
    }
}

void TimingAnalyzer::recordSilentClicks(qint64 tickIndex)
{
    const qint64 silentClicks = std::clamp<qint64>(tickIndex - m_lastResolvedTick - 1, 0, 8);
    m_echoes = static_cast<std::uint8_t>(m_echoes << silentClicks);
    m_lastResolvedTick = tickIndex;
}

void TimingAnalyzer::registerHit(const Hit &detected)
{
    const double offset = detected.match.offset;
    m_lastOffset = static_cast<int>(std::lround(offset * 1000.0));
    if (std::abs(offset) <= kOnTimeSeconds) {
        m_lastAccuracy = OnTime;
    } else {
        m_lastAccuracy = offset < 0.0 ? Early : Late;
    }
    m_hasHit = true;
    markMeasureHit(detected.match.tick);
    emit hit();

    updatePlayedBpm(detected);
}

// The hits stay on their clicks until the first hit of the next measure.
void TimingAnalyzer::markMeasureHit(const TickEvent &tick)
{
    const qsizetype clicks = static_cast<qsizetype>(tick.beatsPerMeasure) * tick.subdivisions;
    if (tick.measure != m_hitsMeasure || m_measureHits.size() != clicks) {
        m_measureHits = QList<int>(clicks, -1);
        m_hitsMeasure = tick.measure;
    }

    const qsizetype click = (static_cast<qsizetype>(tick.beat) * tick.subdivisions) + tick.subdivision;
    if (click < clicks) {
        m_measureHits[click] = m_lastAccuracy;
    }
}

// Each interval between hits is rounded to whole clicks, so the hits may skip clicks and still give the tempo.
void TimingAnalyzer::updatePlayedBpm(const Hit &hit)
{
    const TickMatch &match = hit.match;
    const double time = hit.time;
    const std::optional<double> previous = std::exchange(m_lastHitTime, time);
    if (!previous || time - *previous > maxHitGap()) {
        m_intervals.clear();
        return;
    }

    const double seconds = time - *previous;
    const long steps = std::lround(seconds / match.tickSeconds);
    if (steps == 0) {
        m_lastHitTime = previous;
        return;
    }

    m_intervals.emplace_back(seconds, static_cast<double>(steps) / std::max<int>(1, match.tick.subdivisions));
    if (m_intervals.size() > kBpmIntervals) {
        m_intervals.pop_front();
    }
    if (m_intervals.size() < kMinBpmIntervals) {
        return;
    }

    const auto total = [this](double Interval::*field) {
        return std::ranges::fold_left(m_intervals | std::views::transform(field), 0.0, std::plus{});
    };
    const int bpm = static_cast<int>(std::lround(60.0 * total(&Interval::beats) / total(&Interval::seconds)));
    if (bpm == m_playedBpm) {
        return;
    }
    m_playedBpm = bpm;
    emit playedBpmChanged();
}
