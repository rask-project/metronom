#include "Metronome.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QDebug>
#include <QMediaDevices>
#include <QSettings>
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <numbers>
#include <ranges>
#include <type_traits>

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

constexpr qint32 kMinBpm{40};
constexpr qint32 kMaxBpm{300};
constexpr qint32 kMaxNumerator{12};
constexpr std::array<qint32, 3> kDenominators{2, 4, 8};
constexpr qint32 kMaxSubdivisions{8};
constexpr qint32 kPositionPollMs{4};

// Note lengths in 1/32 of a whole note, so dotted notes are whole numbers.
constexpr qint32 kWholeLength{32};
constexpr std::array<Metronome::NoteValue, 6> kNoteValuesByLength{
    Metronome::Half,
    Metronome::DottedQuarter,
    Metronome::Quarter,
    Metronome::Eighth,
    Metronome::Sixteenth,
    Metronome::ThirtySecond
};

constexpr qint32 noteLength(Metronome::NoteValue noteValue) noexcept
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

constexpr quint32 packTickConfig(qint32 bpm, qint32 subdivisions, qint32 beatsPerMeasure) noexcept
{
    return (static_cast<quint32>(bpm) << 16U) | (static_cast<quint32>(subdivisions) << 8U) |
           static_cast<quint32>(beatsPerMeasure);
}

constexpr quint32 packTickPosition(quint16 serial, qint32 beat, qint32 subdivision) noexcept
{
    return (static_cast<quint32>(serial) << 16U) | (static_cast<quint32>(beat) << 8U) |
           static_cast<quint32>(subdivision);
}

constexpr qint32 unpackField(quint32 packed, quint32 shift, quint32 mask) noexcept
{
    return static_cast<qint32>((packed >> shift) & mask);
}

template <typename T> T fromNormalized(float value) noexcept
{
    if constexpr (std::is_same_v<T, float>) {
        return std::clamp(value, -1.0f, 1.0f);
    }

    const qint32 clamped{std::clamp(static_cast<qint32>(std::lround(value * 32767.0f)), -32768, 32767)};
    return static_cast<qint16>(clamped);
}

QAudioFormat chooseFormat(const QAudioDevice &device)
{
    static constexpr std::array<qint32, 7> sampleRates{44100, 48000, 44000, 32000, 22050, 16000, 8000};
    static constexpr std::array<qint32, 2> channelCounts{2, 1};
    static constexpr std::array<QAudioFormat::SampleFormat, 2> sampleFormats{QAudioFormat::Int16, QAudioFormat::Float};

    for (const qint32 rate : sampleRates) {
        for (const qint32 channels : channelCounts) {
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

QList<float> renderClick(const ClickSound &sound, qint32 sampleRate)
{
    const auto samples{static_cast<qsizetype>(sampleRate * sound.durationSeconds)};
    QList<float> click(samples);
    quint32 seed{1};

    for (qsizetype i{0}; i < samples; ++i) {
        const double t{static_cast<double>(i) / sampleRate};
        const double envelope{std::exp(-t / sound.decaySeconds)};
        seed = (seed * 1664525u) + 1013904223u;
        const double noise{(static_cast<double>(seed >> 8) / 8388608.0) - 1.0};
        const double tone{std::sin(2.0 * std::numbers::pi * sound.freq * t)};
        const double sample{(0.6 * tone) + (0.4 * noise)};
        click[i] = static_cast<float>(sample * envelope * sound.amplitude);
    }

    return click;
}
} // namespace

Metronome::Metronome(QObject *parent)
    : QObject(parent)
    , m_devices(QMediaDevices::audioOutputs())
    , m_currentDevice(defaultDeviceIndex())
{
    m_autoIncrement = QSettings().value("autoIncrement", false).toBool();
    m_autoIncrementInterval = QSettings().value("autoIncrementInterval", 60).toInt();
    m_autoIncrementTimer.setSingleShot(false);
    connect(&m_autoIncrementTimer, &QTimer::timeout, this, &Metronome::nextTempoMarking);

    m_positionTimer.setTimerType(Qt::PreciseTimer);
    m_positionTimer.setInterval(kPositionPollMs);
    connect(&m_positionTimer, &QTimer::timeout, this, &Metronome::pollPosition);

    publishTickConfig();
    rebuildAudio();
}

Metronome::~Metronome()
{
    stop();
}

qint32 Metronome::defaultDeviceIndex() const
{
    const auto it{std::ranges::find_if(m_devices, &QAudioDevice::isDefault)};
    return it == m_devices.end() ? 0 : static_cast<qint32>(std::distance(m_devices.begin(), it));
}

QAudioDevice Metronome::deviceAt(qint32 index) const
{
    if (m_devices.isEmpty()) {
        return QMediaDevices::defaultAudioOutput();
    }
    return m_devices.value(index);
}

void Metronome::rebuildAudio()
{
    const QAudioDevice device{deviceAt(m_currentDevice)};
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
    QList<QString> names;
    names.reserve(m_devices.size());
    std::ranges::transform(m_devices, std::back_inserter(names), &QAudioDevice::description);
    return names;
}

qint32 Metronome::currentAudioDevice() const
{
    return m_currentDevice;
}

void Metronome::setCurrentAudioDevice(qint32 index)
{
    if (index < 0 || index >= m_devices.size()) {
        return;
    }

    if (index == m_currentDevice) {
        return;
    }

    const bool wasRunning{m_running};
    if (wasRunning) {
        stop();
    }

    m_currentDevice = index;
    rebuildAudio();

    emit currentAudioDeviceChanged();

    if (wasRunning) {
        start();
    }
}

qint32 Metronome::bpm() const
{
    return m_bpm;
}

void Metronome::setBpm(qint32 bpm)
{
    const qint32 clamped{std::clamp(bpm, kMinBpm, kMaxBpm)};
    if (clamped == m_bpm) {
        return;
    }

    m_bpm = clamped;
    publishTickConfig();
    emit bpmChanged();
}

qint32 Metronome::minBpm() const
{
    return kMinBpm;
}

qint32 Metronome::maxBpm() const
{
    return kMaxBpm;
}

qint32 Metronome::numerator() const
{
    return m_numerator;
}

void Metronome::setNumerator(qint32 numerator)
{
    if (numerator < 1 || numerator > kMaxNumerator || numerator == m_numerator) {
        return;
    }

    m_numerator = numerator;
    applyTimeSignature();
}

qint32 Metronome::denominator() const
{
    return m_denominator;
}

void Metronome::setDenominator(qint32 denominator)
{
    if (denominator == m_denominator || !std::ranges::contains(kDenominators, denominator)) {
        return;
    }

    m_denominator = denominator;
    applyTimeSignature();
}

QList<qint32> Metronome::numerators() const
{
    QList<qint32> values;
    std::ranges::copy(std::views::iota(1, kMaxNumerator + 1), std::back_inserter(values));
    return values;
}

QList<qint32> Metronome::denominators() const
{
    return {kDenominators.begin(), kDenominators.end()};
}

// 6/8, 9/8 and 12/8 are felt in dotted quarters, each one split in three eighths.
bool Metronome::isCompound() const
{
    return m_denominator == 8 && m_numerator > 3 && m_numerator % 3 == 0;
}

qint32 Metronome::beatsPerMeasure() const
{
    return isCompound() ? m_numerator / 3 : m_numerator;
}

qint32 Metronome::beatLength() const
{
    const qint32 unitLength{kWholeLength / m_denominator};
    return isCompound() ? unitLength * 3 : unitLength;
}

qint32 Metronome::subdivisionsFor(NoteValue noteValue) const
{
    const qint32 length{noteLength(noteValue)};
    const qint32 beat{beatLength()};
    if (length > beat || beat % length != 0) {
        return 0;
    }

    const qint32 subdivisions{beat / length};
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

QList<qint32> Metronome::availableNoteValues() const
{
    QList<qint32> values;
    std::ranges::copy(
        kNoteValuesByLength | std::views::filter([this](NoteValue value) { return subdivisionsFor(value) > 0; }),
        std::back_inserter(values)
    );
    return values;
}

qint32 Metronome::subdivisions() const
{
    return subdivisionsFor(m_noteValue);
}

bool Metronome::running() const
{
    return m_running;
}

qint32 Metronome::currentBeat() const
{
    return m_currentBeat;
}

qint32 Metronome::currentSubdivision() const
{
    return m_currentSubdivision;
}

void Metronome::publishTickConfig()
{
    m_tickConfig.store(packTickConfig(m_bpm, subdivisions(), beatsPerMeasure()));
}

void Metronome::pollPosition()
{
    const quint32 position{m_tickPosition.load()};
    if (position == m_lastTickPosition) {
        return;
    }

    m_lastTickPosition = position;
    m_currentBeat = unpackField(position, 8U, 0xFFU);
    m_currentSubdivision = unpackField(position, 0U, 0xFFU);
    emit ticked();
}

template <typename T> void Metronome::mixActiveClick(QSpan<T> samples, qsizetype from, qsizetype to) noexcept
{
    if (m_activeClick == nullptr) {
        return;
    }

    const QList<float> &click{*m_activeClick};
    const qsizetype count{std::min(to - from, click.size() - m_activeClickPos)};

    for (qsizetype i{0}; i < count; ++i) {
        const T value{fromNormalized<T>(click[m_activeClickPos + i])};
        std::ranges::fill(samples.subspan((from + i) * m_channelCount, m_channelCount), value);
    }

    m_activeClickPos += count;
    if (m_activeClickPos >= click.size()) {
        m_activeClick = nullptr;
    }
}

template <typename T> void Metronome::fillSamples(QSpan<T> samples) noexcept
{
    std::ranges::fill(samples, fromNormalized<T>(0.0f));

    const qsizetype frameCount{samples.size() / m_channelCount};
    qsizetype frame{0};

    while (frame < frameCount) {
        const qint64 tickFrame{static_cast<qint64>(m_nextTickFrame) - m_framesWritten};
        const auto end{static_cast<qsizetype>(std::clamp<qint64>(tickFrame, frame, frameCount))};
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
    const quint32 config{m_tickConfig.load()};
    const qint32 bpm{unpackField(config, 16U, 0xFFFFU)};
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

    const bool downbeat{m_audioBeat == 0};
    const QList<float> &beatClick{downbeat ? m_accentClick : m_beatClick};
    m_activeClick = m_audioSubdivision == 0 ? &beatClick : &m_subdivisionClick;
    m_activeClickPos = 0;

    ++m_tickSerial;
    m_tickPosition.store(packTickPosition(m_tickSerial, m_audioBeat, m_audioSubdivision));

    ++m_audioSubdivision;
    if (m_audioSubdivision < m_audioSubdivisions) {
        m_nextTickFrame = m_beatStartFrame + (m_beatFrames * m_audioSubdivision / m_audioSubdivisions);
        return;
    }

    m_audioSubdivision = 0;
    m_audioBeat = (m_audioBeat + 1) % m_audioBeatsPerMeasure;
    m_nextTickFrame = m_beatStartFrame + m_beatFrames;
}

void Metronome::start()
{
    if (m_running) {
        return;
    }

    if (m_format.sampleFormat() != QAudioFormat::Int16 && m_format.sampleFormat() != QAudioFormat::Float) {
        qWarning() << "Unsupported audio sample format";
        return;
    }

    m_activeClick = nullptr;
    m_activeClickPos = 0;
    m_nextTickFrame = 0.0;
    m_framesWritten = 0;
    m_audioBeat = 0;
    m_audioSubdivision = 0;
    m_tickSerial = 0;
    m_tickPosition.store(0);
    m_lastTickPosition = 0;
    m_currentBeat = 0;
    m_currentSubdivision = 0;

    m_running = true;
    m_screenLock.setInhibited(true);

    if (m_format.sampleFormat() == QAudioFormat::Int16) {
        m_sink->start([this](QSpan<qint16> samples) { fillSamples(samples); });
    } else {
        m_sink->start([this](QSpan<float> samples) { fillSamples(samples); });
    }

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

QList<qint32> Metronome::tempoMarkings() const
{
    return QList<qint32>(std::begin(tempoMarkingsValues), std::end(tempoMarkingsValues));
}

void Metronome::nextTempoMarking()
{
    const auto it{std::ranges::upper_bound(tempoMarkingsValues, m_bpm)};
    if (it != std::ranges::end(tempoMarkingsValues)) {
        setBpm(*it);
    }
}

void Metronome::previousTempoMarking()
{
    const auto it{std::ranges::lower_bound(tempoMarkingsValues, m_bpm)};
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

qint32 Metronome::autoIncrementInterval() const
{
    return m_autoIncrementInterval;
}

void Metronome::setAutoIncrementInterval(qint32 seconds)
{
    if (m_autoIncrementInterval == seconds) {
        return;
    }

    m_autoIncrementInterval = seconds;
    QSettings().setValue("autoIncrementInterval", seconds);
    emit autoIncrementIntervalChanged();
    updateAutoIncrementTimer();
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

    const auto begin{std::ranges::begin(tempoMarkingsValues)};
    const auto it{std::ranges::upper_bound(tempoMarkingsValues, m_bpm)};
    const qint32 index{(it == begin) ? 0 : static_cast<qint32>(it - begin) - 1};
    return QString::fromUtf8(tempoMarkingNames[index]);
}
