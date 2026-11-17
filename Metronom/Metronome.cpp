#include "Metronome.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QDebug>
#include <QMediaDevices>
#include <QSettings>
#include <QSpan>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <numbers>
#include <type_traits>

namespace
{
constexpr double kStrongFreq = 3400.0;
constexpr double kWeakFreq = 2200.0;
constexpr double kStrongDurationSeconds = 0.012;
constexpr double kWeakDurationSeconds = 0.009;
constexpr double kStrongDecaySeconds = 0.0025;
constexpr double kWeakDecaySeconds = 0.002;
constexpr double kStrongAmplitude = 0.9;
constexpr double kWeakAmplitude = 0.55;
constexpr int kMinBpm = 40;
constexpr int kMaxBpm = 300;

template <typename T> float toNormalized(T value)
{
    if constexpr (std::is_same_v<T, float>) {
        return value;
    }

    return static_cast<float>(value) / 32768.0f;
}

template <typename T> T fromNormalized(float value)
{
    if constexpr (std::is_same_v<T, float>) {
        return qBound(-1.0f, value, 1.0f);
    }

    const int clamped = qBound(-32768, static_cast<int>(std::lround(value * 32767.0f)), 32767);
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
} // namespace

std::vector<float>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Metronome::renderClick(double freq, double durationSeconds, double decaySeconds, double amplitude, int sampleRate)
{
    const int samples = static_cast<int>(sampleRate * durationSeconds);
    std::vector<float> click(samples);
    uint32_t seed = 1;

    for (int i = 0; i < samples; ++i) {
        const double t = static_cast<double>(i) / sampleRate;
        const double envelope = std::exp(-t / decaySeconds);
        seed = (seed * 1664525u) + 1013904223u;
        const double noise = (static_cast<double>(seed >> 8) / 8388608.0) - 1.0;
        const double tone = std::sin(2.0 * std::numbers::pi * freq * t);
        const double sample = (0.6 * tone) + (0.4 * noise);
        click[i] = static_cast<float>(sample * envelope * amplitude);
    }

    return click;
}

Metronome::Metronome(QObject *parent)
    : QObject(parent)
    , m_devices(QMediaDevices::audioOutputs())
    , m_currentDevice(defaultDeviceIndex())
{
    m_autoIncrement = QSettings().value("autoIncrement", false).toBool();
    m_autoIncrementInterval = QSettings().value("autoIncrementInterval", 60).toInt();
    m_autoIncrementTimer.setSingleShot(false);
    connect(&m_autoIncrementTimer, &QTimer::timeout, this, &Metronome::nextTempoMarking);
    rebuildAudio();
}

Metronome::~Metronome()
{
    stop();
}

int Metronome::defaultDeviceIndex() const
{
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices.at(i).isDefault()) {
            return i;
        }
    }
    return 0;
}

QAudioDevice Metronome::deviceAt(int index) const
{
    if (m_devices.isEmpty()) {
        return QMediaDevices::defaultAudioOutput();
    }
    return m_devices.value(index);
}

void Metronome::rebuildAudio()
{
    const QAudioDevice device = deviceAt(m_currentDevice);
    m_format = chooseFormat(device);
    m_sampleRate = m_format.sampleRate();
    m_channelCount = m_format.channelCount();
    m_strongClick =
        renderClick(kStrongFreq, kStrongDurationSeconds, kStrongDecaySeconds, kStrongAmplitude, m_sampleRate);
    m_weakClick = renderClick(kWeakFreq, kWeakDurationSeconds, kWeakDecaySeconds, kWeakAmplitude, m_sampleRate);
    m_tickIntervalSamples = tickIntervalSeconds() * m_sampleRate;
    m_sink = std::make_unique<QAudioSink>(device, m_format);
}

QList<QString> Metronome::audioDeviceNames() const
{
    QList<QString> names;
    names.reserve(m_devices.size());
    for (const QAudioDevice &device : m_devices) {
        names.append(device.description());
    }
    return names;
}

int Metronome::currentAudioDevice() const
{
    return m_currentDevice;
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
    rebuildAudio();

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
    m_tickIntervalSamples = tickIntervalSeconds() * m_sampleRate;
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

Metronome::NoteValue Metronome::noteValue() const
{
    return m_noteValue;
}

void Metronome::setNoteValue(NoteValue noteValue)
{
    if (noteValue == m_noteValue) {
        return;
    }

    m_noteValue = noteValue;
    m_tickIntervalSamples = tickIntervalSeconds() * m_sampleRate;
    emit noteValueChanged();
}

bool Metronome::running() const
{
    return m_running;
}

int Metronome::currentBeat() const
{
    return m_currentBeat;
}

int Metronome::beatsPerMeasure() const
{
    return beatsPerBar * subdivision();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
template <typename T> void Metronome::writeClickT(T *data, int frameCount, int offset, const std::vector<float> &click)
{
    for (std::size_t i = 0; i < click.size(); ++i) {
        const int pos = offset + static_cast<int>(i);
        if (pos >= frameCount) {
            m_pendingTail.assign(click.begin() + static_cast<std::ptrdiff_t>(i), click.end());
            return;
        }

        for (int c = 0; c < m_channelCount; ++c) {
            const int index = (pos * m_channelCount) + c;
            data[index] = fromNormalized<T>(toNormalized<T>(data[index]) + click[i]);
        }
    }
}

template <typename T> void Metronome::fillSamplesT(T *data, int count)
{
    const int frameCount = count / m_channelCount;
    std::fill_n(data, count, fromNormalized<T>(0.0f));

    for (int i = 0; i < static_cast<int>(m_pendingTail.size()) && i < frameCount; ++i) {
        for (int c = 0; c < m_channelCount; ++c) {
            data[(i * m_channelCount) + c] = fromNormalized<T>(m_pendingTail[i]);
        }
    }

    m_pendingTail.clear();

    const double bufferStart = static_cast<double>(m_framesWritten);
    const double bufferEnd = bufferStart + frameCount;
    const int measure = beatsPerMeasure();

    while (m_nextTickSample < bufferEnd) {
        const int offset = static_cast<int>(m_nextTickSample - bufferStart);
        if (offset >= 0 && offset < frameCount) {
            const bool strong = (m_beatIndex % measure) == 0;
            writeClickT<T>(data, frameCount, offset, strong ? m_strongClick : m_weakClick);
        }

        const int beat = m_beatIndex % measure;
        if (beat != m_currentBeat) {
            m_currentBeat = beat;
            emit currentBeatChanged();
        }

        m_nextTickSample += m_tickIntervalSamples;
        m_beatIndex++;
    }

    m_framesWritten = static_cast<qint64>(bufferEnd);
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

    resetScheduler();
    m_running = true;
    m_screenLock.setInhibited(true);
    m_currentBeat = 0;

    if (m_format.sampleFormat() == QAudioFormat::Int16) {
        m_sink->start([this](QSpan<qint16> samples) {
            fillSamplesT<qint16>(samples.data(), static_cast<int>(samples.size()));
        });
    } else {
        m_sink->start([this](QSpan<float> samples) {
            fillSamplesT<float>(samples.data(), static_cast<int>(samples.size()));
        });
    }

    emit runningChanged();
    emit currentBeatChanged();
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

int Metronome::subdivision() const
{
    switch (m_noteValue) {
    case Eighth:
        return 2;
    case Sixteenth:
        return 4;
    case ThirtySecond:
        return 8;
    case Quarter:
    default:
        return 1;
    }
}

double Metronome::tickIntervalSeconds() const
{
    return (60.0 / static_cast<double>(m_bpm)) / static_cast<double>(subdivision());
}

void Metronome::resetScheduler()
{
    m_nextTickSample = 0.0;
    m_framesWritten = 0;
    m_beatIndex = 0;
    m_pendingTail.clear();
    m_tickIntervalSamples = tickIntervalSeconds() * m_sampleRate;
}
