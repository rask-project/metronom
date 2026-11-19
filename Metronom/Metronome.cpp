#include "Metronome.h"

#include "AudioDevices.h"
#include "AudioOutputInfo.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QDebug>
#include <QMediaDevices>
#include <QSettings>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <numbers>
#include <ranges>
#include <type_traits>
#include <utility>

namespace
{
struct ClickSound {
    double freq;
    double durationSeconds;
    double decaySeconds;
    double amplitude;
};

constexpr ClickSound kAccentClick{.freq = 3400.0, .durationSeconds = 0.012, .decaySeconds = 0.0025, .amplitude = 0.9};
constexpr ClickSound kBeatClick{.freq = 2200.0, .durationSeconds = 0.009, .decaySeconds = 0.002, .amplitude = 0.6};
constexpr ClickSound
    kSubdivisionClick{.freq = 1600.0, .durationSeconds = 0.006, .decaySeconds = 0.0015, .amplitude = 0.3};

constexpr int kMinBpm = 40;
constexpr int kMaxBpm = 300;
constexpr int kMaxNumerator = 12;
constexpr std::array<int, 3> kDenominators{2, 4, 8};
constexpr int kMaxSubdivisions = 8;
constexpr int kPositionPollMs = 4;
constexpr int kCalibrationBpm = 120;

// Note lengths in 1/32 of a whole note, so dotted notes are whole numbers.
constexpr int kWholeLength = 32;
constexpr std::array<Metronome::NoteValue, 6> kNoteValuesByLength{
    Metronome::Half,
    Metronome::DottedQuarter,
    Metronome::Quarter,
    Metronome::Eighth,
    Metronome::Sixteenth,
    Metronome::ThirtySecond
};

constexpr int noteLength(Metronome::NoteValue noteValue) noexcept
{
    switch (noteValue) {
    case Metronome::Half:
        return 16;
    case Metronome::DottedQuarter:
        return 12;
    case Metronome::Quarter:
        return 8;
    case Metronome::Eighth:
        return 4;
    case Metronome::Sixteenth:
        return 2;
    case Metronome::ThirtySecond:
        return 1;
    }
    return kWholeLength;
}

constexpr std::uint32_t packTickConfig(int bpm, int subdivisions, int beatsPerMeasure) noexcept
{
    return (static_cast<std::uint32_t>(bpm) << 16U) | (static_cast<std::uint32_t>(subdivisions) << 8U) |
           static_cast<std::uint32_t>(beatsPerMeasure);
}

constexpr std::uint32_t packTickPosition(std::uint16_t serial, int beat, int subdivision) noexcept
{
    return (static_cast<std::uint32_t>(serial) << 16U) | (static_cast<std::uint32_t>(beat) << 8U) |
           static_cast<std::uint32_t>(subdivision);
}

constexpr int unpackField(std::uint32_t packed, unsigned shift, std::uint32_t mask) noexcept
{
    return static_cast<int>((packed >> shift) & mask);
}

template <typename T> T fromNormalized(float value) noexcept
{
    if constexpr (std::is_same_v<T, float>) {
        return std::clamp(value, -1.0f, 1.0f);
    }

    const int clamped = std::clamp(static_cast<int>(std::lround(value * 32767.0f)), -32768, 32767);
    return static_cast<qint16>(clamped);
}

QAudioFormat chooseFormat(const QAudioDevice &device)
{
    static constexpr std::array<int, 7> sampleRates{44100, 48000, 44000, 32000, 22050, 16000, 8000};
    static constexpr std::array<int, 2> channelCounts{2, 1};
    static constexpr std::array<QAudioFormat::SampleFormat, 2> sampleFormats{QAudioFormat::Int16, QAudioFormat::Float};

    for (const int rate : sampleRates) {
        for (const int channels : channelCounts) {
            for (const auto sampleFormat : sampleFormats) {
                QAudioFormat format;
                format.setSampleRate(rate);
                format.setChannelCount(channels);
                format.setSampleFormat(sampleFormat);

                if (device.isFormatSupported(format)) {
                    return format;
                }
            }
        }
    }

    return device.preferredFormat();
}

std::vector<float> renderClick(const ClickSound &sound, int sampleRate)
{
    const int samples = static_cast<int>(sampleRate * sound.durationSeconds);
    std::vector<float> click(samples);
    uint32_t seed = 1;

    for (int i = 0; i < samples; ++i) {
        const double t = static_cast<double>(i) / sampleRate;
        const double envelope = std::exp(-t / sound.decaySeconds);
        seed = (seed * 1664525u) + 1013904223u;
        const double noise = (static_cast<double>(seed >> 8) / 8388608.0) - 1.0;
        const double tone = std::sin(2.0 * std::numbers::pi * sound.freq * t);
        const double sample = (0.6 * tone) + (0.4 * noise);
        click[i] = static_cast<float>(sample * envelope * sound.amplitude);
    }

    return click;
}
} // namespace

Metronome::Metronome(QObject *parent)
    : QObject(parent)
    , m_devices(QMediaDevices::audioOutputs())
    , m_currentDevice(AudioDevices::chosenOrDefaultIndex(m_devices, QSettings().value("audioOutput").toByteArray()))
{
    m_autoIncrement = QSettings().value("autoIncrement", false).toBool();
    m_autoIncrementInterval = QSettings().value("autoIncrementInterval", 60).toInt();
    m_autoIncrementTimer.setSingleShot(false);
    connect(&m_autoIncrementTimer, &QTimer::timeout, this, &Metronome::nextTempoMarking);

    m_positionTimer.setTimerType(Qt::PreciseTimer);
    m_positionTimer.setInterval(kPositionPollMs);
    connect(&m_positionTimer, &QTimer::timeout, this, &Metronome::pollPosition);

    // Queued: finishing may restart the audio, which stops the analyzer that is emitting.
    connect(&m_timing, &TimingAnalyzer::calibrationFinished, this, &Metronome::finishCalibration, Qt::QueuedConnection);
    connect(&m_timing, &TimingAnalyzer::enabledChanged, this, &Metronome::restartIfRunning);
    connect(&m_timing, &TimingAnalyzer::microphoneGranted, this, &Metronome::restartIfRunning);
    connect(&m_timing, &TimingAnalyzer::inputDeviceSwitched, this, &Metronome::restartIfRunning);
    connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &Metronome::refreshAudioDevices);
    connect(&m_mediaDevices, &QMediaDevices::audioInputsChanged, this, &Metronome::refreshAudioDevices);

    publishTickConfig();
    rebuildAudio(deviceAt(m_currentDevice));
    m_timing.setOutputDevice(deviceAt(m_currentDevice).id());
}

Metronome::~Metronome()
{
    stop();
}

QAudioDevice Metronome::deviceAt(int index) const
{
    if (m_devices.isEmpty()) {
        return QMediaDevices::defaultAudioOutput();
    }
    return m_devices.value(index);
}

void Metronome::rebuildAudio(const QAudioDevice &device)
{
    m_sinkDeviceId = device.id();
    m_format = chooseFormat(device);
    m_sampleRate = m_format.sampleRate();
    m_channelCount = m_format.channelCount();
    m_accentClick = renderClick(kAccentClick, m_sampleRate);
    m_beatClick = renderClick(kBeatClick, m_sampleRate);
    m_subdivisionClick = renderClick(kSubdivisionClick, m_sampleRate);
    m_sink = std::make_unique<QAudioSink>(device, m_format);
}

QList<QString> Metronome::audioDeviceNames() const
{
    return AudioDevices::names(m_devices);
}

int Metronome::currentAudioDevice() const
{
    return m_currentDevice;
}

void Metronome::restartIfRunning()
{
    if (!m_running) {
        return;
    }
    stop();
    start();
}

void Metronome::refreshAudioDevices()
{
    m_timing.refreshInputDevices();

    const QByteArray previousId = deviceAt(m_currentDevice).id();
    m_devices = QMediaDevices::audioOutputs();
    m_currentDevice = AudioDevices::chosenOrDefaultIndex(m_devices, QSettings().value("audioOutput").toByteArray());
    emit audioDevicesChanged();
    emit currentAudioDeviceChanged();

    if (deviceAt(m_currentDevice).id() == previousId) {
        return;
    }

    const bool wasRunning = m_running;
    stop();
    rebuildAudio(deviceAt(m_currentDevice));
    m_timing.setOutputDevice(deviceAt(m_currentDevice).id());
    if (wasRunning) {
        start();
    }
}

void Metronome::setCurrentAudioDevice(int index)
{
    if (index < 0 || index >= m_devices.size()) {
        return;
    }

    if (index == m_currentDevice) {
        return;
    }

    const bool wasRunning = m_running;
    if (wasRunning) {
        stop();
    }

    m_currentDevice = index;
    QSettings().setValue("audioOutput", deviceAt(m_currentDevice).id());
    rebuildAudio(deviceAt(m_currentDevice));
    m_timing.setOutputDevice(deviceAt(m_currentDevice).id());

    emit currentAudioDeviceChanged();

    if (wasRunning) {
        start();
    }
}

int Metronome::bpm() const
{
    return m_bpm;
}

void Metronome::setBpm(int bpm)
{
    const int clamped = std::clamp(bpm, kMinBpm, kMaxBpm);
    if (clamped == m_bpm) {
        return;
    }

    m_bpm = clamped;
    publishTickConfig();
    emit bpmChanged();
}

int Metronome::minBpm() const
{
    return kMinBpm;
}

int Metronome::maxBpm() const
{
    return kMaxBpm;
}

int Metronome::numerator() const
{
    return m_numerator;
}

void Metronome::setNumerator(int numerator)
{
    if (numerator < 1 || numerator > kMaxNumerator || numerator == m_numerator) {
        return;
    }

    m_numerator = numerator;
    applyTimeSignature();
}

int Metronome::denominator() const
{
    return m_denominator;
}

void Metronome::setDenominator(int denominator)
{
    if (denominator == m_denominator || !std::ranges::contains(kDenominators, denominator)) {
        return;
    }

    m_denominator = denominator;
    applyTimeSignature();
}

QList<int> Metronome::numerators() const
{
    QList<int> values;
    std::ranges::copy(std::views::iota(1, kMaxNumerator + 1), std::back_inserter(values));
    return values;
}

QList<int> Metronome::denominators() const
{
    return {kDenominators.begin(), kDenominators.end()};
}

// 6/8, 9/8 and 12/8 are felt in dotted quarters, each one split in three eighths.
bool Metronome::isCompound() const
{
    return m_denominator == 8 && m_numerator > 3 && m_numerator % 3 == 0;
}

int Metronome::beatsPerMeasure() const
{
    return isCompound() ? m_numerator / 3 : m_numerator;
}

int Metronome::beatLength() const
{
    const int unitLength = kWholeLength / m_denominator;
    return isCompound() ? unitLength * 3 : unitLength;
}

int Metronome::subdivisionsFor(NoteValue noteValue) const
{
    const int length = noteLength(noteValue);
    const int beat = beatLength();
    if (length > beat || beat % length != 0) {
        return 0;
    }

    const int subdivisions = beat / length;
    return subdivisions <= kMaxSubdivisions ? subdivisions : 0;
}

void Metronome::applyTimeSignature()
{
    if (subdivisionsFor(m_noteValue) == 0) {
        m_noteValue =
            *std::ranges::find_if(kNoteValuesByLength, [this](NoteValue value) { return subdivisionsFor(value) > 0; });
        emit noteValueChanged();
    }

    publishTickConfig();
    emit timeSignatureChanged();
    emit subdivisionsChanged();
}

Metronome::NoteValue Metronome::noteValue() const
{
    return m_noteValue;
}

void Metronome::setNoteValue(NoteValue noteValue)
{
    if (noteValue == m_noteValue || subdivisionsFor(noteValue) == 0) {
        return;
    }

    m_noteValue = noteValue;
    publishTickConfig();
    emit noteValueChanged();
    emit subdivisionsChanged();
}

QList<int> Metronome::availableNoteValues() const
{
    QList<int> values;
    std::ranges::copy(
        kNoteValuesByLength | std::views::filter([this](NoteValue value) { return subdivisionsFor(value) > 0; }),
        std::back_inserter(values)
    );
    return values;
}

int Metronome::subdivisions() const
{
    return subdivisionsFor(m_noteValue);
}

bool Metronome::running() const
{
    return m_running;
}

int Metronome::currentBeat() const
{
    return m_currentBeat;
}

int Metronome::currentSubdivision() const
{
    return m_currentSubdivision;
}

// Calibration: one accented click per beat at a fixed tempo, so each click is easy to tell apart.
void Metronome::publishTickConfig()
{
    if (m_calibrating) {
        m_tickConfig.store(packTickConfig(kCalibrationBpm, 1, 1), std::memory_order_release);
        return;
    }
    m_tickConfig.store(packTickConfig(m_bpm, subdivisions(), beatsPerMeasure()), std::memory_order_release);
}

// A tick is published when its click is written; it is shown once the output delay has passed, when it is heard.
void Metronome::pollPosition()
{
    const auto now = std::chrono::steady_clock::now();
    const std::uint32_t position = m_tickPosition.load(std::memory_order_acquire);
    if (position != m_lastTickPosition) {
        m_lastTickPosition = position;
        m_heardPositions.emplace_back(position, now + m_outputDelay);
    }

    while (!m_heardPositions.empty() && m_heardPositions.front().second <= now) {
        applyPosition(m_heardPositions.front().first);
        m_heardPositions.pop_front();
    }
}

void Metronome::applyPosition(std::uint32_t position)
{
    m_currentBeat = unpackField(position, 8U, 0xFFU);
    m_currentSubdivision = unpackField(position, 0U, 0xFFU);
    emit ticked();
}

template <typename T> void Metronome::mixActiveClick(QSpan<T> samples, qint64 from, qint64 to) noexcept
{
    if (m_activeClick == nullptr) {
        return;
    }

    const std::vector<float> &click = *m_activeClick;
    const auto count = std::min(to - from, static_cast<qint64>(click.size() - m_activeClickPos));

    for (qint64 i = 0; i < count; ++i) {
        const T value = fromNormalized<T>(click[m_activeClickPos + static_cast<std::size_t>(i)]);
        std::ranges::fill(samples.subspan((from + i) * m_channelCount, m_channelCount), value);
    }

    m_activeClickPos += static_cast<std::size_t>(count);
    if (m_activeClickPos >= click.size()) {
        m_activeClick = nullptr;
    }
}

template <typename T> void Metronome::fillSamples(QSpan<T> samples) noexcept
{
    std::ranges::fill(samples, fromNormalized<T>(0.0f));

    const qint64 frameCount = samples.size() / m_channelCount;
    qint64 frame = 0;
    m_timing.outputCallback(m_framesWritten);

    while (frame < frameCount) {
        const qint64 tickFrame = static_cast<qint64>(m_nextTickFrame) - m_framesWritten;
        const qint64 end = std::clamp(tickFrame, frame, frameCount);
        mixActiveClick(samples, frame, end);
        frame = end;

        if (frame < frameCount) {
            startTick();
        }
    }

    m_framesWritten += frameCount;
}

// Settings are latched at each beat, so a change never breaks the beat grid.
void Metronome::startBeat() noexcept
{
    const std::uint32_t config = m_tickConfig.load(std::memory_order_acquire);
    const int bpm = unpackField(config, 16U, 0xFFFFU);
    m_audioSubdivisions = unpackField(config, 8U, 0xFFU);
    m_audioBeatsPerMeasure = unpackField(config, 0U, 0xFFU);
    m_beatFrames = 60.0 * m_sampleRate / bpm;
    m_beatStartFrame = m_nextTickFrame;

    if (m_audioBeat >= m_audioBeatsPerMeasure) {
        m_audioBeat = 0;
    }
}

void Metronome::startTick() noexcept
{
    if (m_audioSubdivision == 0) {
        startBeat();
    }

    const bool downbeat = m_audioBeat == 0;
    const std::vector<float> &beatClick = downbeat ? m_accentClick : m_beatClick;
    m_activeClick = m_audioSubdivision == 0 ? &beatClick : &m_subdivisionClick;
    m_activeClickPos = 0;
    m_timing.tickPlayed(
        {.frame = static_cast<qint64>(m_nextTickFrame),
         .bpm = static_cast<int>(std::lround(60.0 * m_sampleRate / m_beatFrames)),
         .beatsPerMeasure = m_audioBeatsPerMeasure,
         .subdivisions = m_audioSubdivisions,
         .beat = m_audioBeat,
         .subdivision = m_audioSubdivision}
    );

    ++m_tickSerial;
    m_tickPosition.store(packTickPosition(m_tickSerial, m_audioBeat, m_audioSubdivision), std::memory_order_release);

    ++m_audioSubdivision;
    if (m_audioSubdivision < m_audioSubdivisions) {
        m_nextTickFrame = m_beatStartFrame + (m_beatFrames * m_audioSubdivision / m_audioSubdivisions);
        return;
    }

    m_audioSubdivision = 0;
    m_audioBeat = (m_audioBeat + 1) % m_audioBeatsPerMeasure;
    m_nextTickFrame = m_beatStartFrame + m_beatFrames;
}

// The microphone has to hear the calibration clicks: the device's own speaker plays them, else the system output.
QAudioDevice Metronome::calibrationOutput() const
{
    if (const auto speakerId = AudioOutputInfo::builtInSpeakerId()) {
        if (const auto index = AudioDevices::indexOf(m_devices, *speakerId)) {
            return m_devices.value(*index);
        }
    }
    return QMediaDevices::defaultAudioOutput();
}

// Calibrating on another output than the chosen one (headphones) adds the difference of their reported latencies.
bool Metronome::useCalibrationOutput()
{
    m_timing.setLatencyAdjustment(0.0);
    if (!m_timing.needsCalibration() || m_calibrationFailed) {
        return false;
    }

    const QAudioDevice chosen = deviceAt(m_currentDevice);
    const QAudioDevice output = calibrationOutput();
    if (output.isNull() || output.id() == chosen.id()) {
        return false;
    }

    const auto chosenLatency = AudioOutputInfo::latencySeconds(chosen.id());
    const auto outputLatency = AudioOutputInfo::latencySeconds(output.id());
    if (chosenLatency && outputLatency) {
        m_timing.setLatencyAdjustment(*chosenLatency - *outputLatency);
    }
    rebuildAudio(output);
    return true;
}

void Metronome::start()
{
    if (m_running) {
        return;
    }

    const bool onOtherOutput = useCalibrationOutput();

    if (m_format.sampleFormat() != QAudioFormat::Int16 && m_format.sampleFormat() != QAudioFormat::Float) {
        qWarning() << "Unsupported audio sample format";
        if (onOtherOutput) {
            rebuildAudio(deviceAt(m_currentDevice));
        }
        return;
    }

    m_activeClick = nullptr;
    m_activeClickPos = 0;
    m_nextTickFrame = 0.0;
    m_framesWritten = 0;
    m_audioBeat = 0;
    m_audioSubdivision = 0;
    m_tickSerial = 0;
    m_tickPosition.store(0, std::memory_order_relaxed);
    m_lastTickPosition = 0;
    m_currentBeat = 0;
    m_currentSubdivision = 0;

    m_calibrating = m_timing.start(m_sampleRate, !std::exchange(m_calibrationFailed, false));
    publishTickConfig();
    if (onOtherOutput && !m_calibrating) {
        rebuildAudio(deviceAt(m_currentDevice));
    }
    m_calibratingOnOtherOutput = onOtherOutput && m_calibrating;

    m_running = true;
    m_screenLock.setInhibited(true);

    if (m_format.sampleFormat() == QAudioFormat::Int16) {
        m_sink->start([this](QSpan<qint16> samples) { fillSamples(samples); });
    } else {
        m_sink->start([this](QSpan<float> samples) { fillSamples(samples); });
    }

    // What the system reports for the device (macOS); the callback sink adds no buffering of its own.
    m_outputDelay = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(AudioOutputInfo::latencySeconds(m_sinkDeviceId).value_or(0.0))
    );
    m_heardPositions.clear();
    m_positionTimer.start();
    emit runningChanged();
    updateAutoIncrementTimer();
}

void Metronome::toggle()
{
    if (m_running) {
        stop();
        return;
    }
    start();
}

void Metronome::stop()
{
    if (!m_running) {
        return;
    }

    m_running = false;
    m_screenLock.setInhibited(false);

    m_sink->stop();
    m_positionTimer.stop();
    m_timing.stop();
    m_calibrating = false;
    publishTickConfig();
    if (m_calibratingOnOtherOutput) {
        m_calibratingOnOtherOutput = false;
        rebuildAudio(deviceAt(m_currentDevice));
    }
    emit runningChanged();
    updateAutoIncrementTimer();
}

void Metronome::increaseBpm()
{
    setBpm(m_bpm + 1);
}

void Metronome::decreaseBpm()
{
    setBpm(m_bpm - 1);
}

QList<int> Metronome::tempoMarkings() const
{
    return QList<int>(std::begin(tempoMarkingsValues), std::end(tempoMarkingsValues));
}

void Metronome::nextTempoMarking()
{
    const auto it = std::ranges::upper_bound(tempoMarkingsValues, m_bpm);
    if (it != std::ranges::end(tempoMarkingsValues)) {
        setBpm(*it);
    }
}

void Metronome::previousTempoMarking()
{
    const auto it = std::ranges::lower_bound(tempoMarkingsValues, m_bpm);
    if (it != std::ranges::begin(tempoMarkingsValues)) {
        setBpm(*std::prev(it));
    }
}

bool Metronome::autoIncrement() const
{
    return m_autoIncrement;
}

void Metronome::setAutoIncrement(bool enabled)
{
    if (m_autoIncrement == enabled) {
        return;
    }

    m_autoIncrement = enabled;
    QSettings().setValue("autoIncrement", enabled);
    emit autoIncrementChanged();
    updateAutoIncrementTimer();
}

int Metronome::autoIncrementInterval() const
{
    return m_autoIncrementInterval;
}

void Metronome::setAutoIncrementInterval(int seconds)
{
    if (m_autoIncrementInterval == seconds) {
        return;
    }

    m_autoIncrementInterval = seconds;
    QSettings().setValue("autoIncrementInterval", seconds);
    emit autoIncrementIntervalChanged();
    updateAutoIncrementTimer();
}

TimingAnalyzer *Metronome::timing()
{
    return &m_timing;
}

void Metronome::finishCalibration()
{
    m_calibrating = false;
    publishTickConfig();
    if (!m_calibratingOnOtherOutput) {
        return;
    }
    // Back on the chosen output; a failed calibration is tried again on the next play.
    m_calibrationFailed = !m_timing.calibrated();
    restartIfRunning();
}

void Metronome::updateAutoIncrementTimer()
{
    if (m_running && m_autoIncrement) {
        m_autoIncrementTimer.start(m_autoIncrementInterval * 1000);
    } else {
        m_autoIncrementTimer.stop();
    }
}

QString Metronome::tempoMarking() const
{
    static_assert(std::size(tempoMarkingsValues) == tempoMarkingNames.size());

    const auto begin = std::ranges::begin(tempoMarkingsValues);
    const auto it = std::ranges::upper_bound(tempoMarkingsValues, m_bpm);
    const int index = (it == begin) ? 0 : static_cast<int>(it - begin) - 1;
    return QString::fromUtf8(tempoMarkingNames[index]);
}
