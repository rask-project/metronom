#include "Metronome.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QSpan>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

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
} // namespace

Metronome::Metronome(QObject *parent)
    : QObject(parent)
    , m_strongClick(renderClick(kStrongFreq, kStrongDurationSeconds, kStrongDecaySeconds, kStrongAmplitude))
    , m_weakClick(renderClick(kWeakFreq, kWeakDurationSeconds, kWeakDecaySeconds, kWeakAmplitude))
    , m_tickIntervalSamples(tickIntervalSeconds() * SampleRate)
{
    QAudioFormat format;
    format.setSampleRate(SampleRate);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    m_sink = std::make_unique<QAudioSink>(format);
}

Metronome::~Metronome()
{
    stop();
}

int Metronome::bpm() const
{
    return m_bpm;
}

void Metronome::setBpm(int bpm)
{
    const int clamped = std::clamp(bpm, MinBpm, MaxBpm);
    if (clamped == m_bpm) {
        return;
    }

    m_bpm = clamped;
    m_tickIntervalSamples = tickIntervalSeconds() * SampleRate;
    emit bpmChanged();
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
    m_tickIntervalSamples = tickIntervalSeconds() * SampleRate;
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
    return BeatsPerBar * subdivision();
}

void Metronome::start()
{
    if (m_running) {
        return;
    }
    resetScheduler();
    m_running = true;
    m_currentBeat = 0;
    m_sink->start([this](QSpan<qint16> samples) { fillSamples(samples.data(), static_cast<int>(samples.size())); });
    emit runningChanged();
    emit currentBeatChanged();
}

void Metronome::stop()
{
    if (!m_running) {
        return;
    }
    m_running = false;
    m_sink->stop();
    emit runningChanged();
}

void Metronome::increaseBpm()
{
    setBpm(m_bpm + 1);
}

void Metronome::decreaseBpm()
{
    setBpm(m_bpm - 1);
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
    m_tickIntervalSamples = tickIntervalSeconds() * SampleRate;
}

void Metronome::fillSamples(qint16 *data, int count)
{
    const int frameCount = count / 2;
    std::fill_n(data, count, static_cast<qint16>(0));

    for (int i = 0; i < static_cast<int>(m_pendingTail.size()) && i < frameCount; ++i) {
        data[i * 2] = m_pendingTail[i];
        data[(i * 2) + 1] = m_pendingTail[i];
    }
    m_pendingTail.clear();

    const double bufferStart = static_cast<double>(m_framesWritten);
    const double bufferEnd = bufferStart + frameCount;
    const int measure = beatsPerMeasure();

    while (m_nextTickSample < bufferEnd) {
        const int offset = static_cast<int>(m_nextTickSample - bufferStart);
        if (offset >= 0 && offset < frameCount) {
            const bool strong = (m_beatIndex % measure) == 0;
            writeClick(data, frameCount, offset, strong ? m_strongClick : m_weakClick);
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

void Metronome::writeClick(qint16 *data, int frameCount, int offset, const std::vector<qint16> &click)
{
    for (std::size_t i = 0; i < click.size(); ++i) {
        const int pos = offset + static_cast<int>(i);
        if (pos >= frameCount) {
            m_pendingTail.assign(click.begin() + static_cast<std::ptrdiff_t>(i), click.end());
            return;
        }
        const qint32 value = static_cast<qint32>(data[pos * 2]) + click[i];
        const qint16 clamped = static_cast<qint16>(qBound<qint32>(-32768, value, 32767));
        data[pos * 2] = clamped;
        data[(pos * 2) + 1] = clamped;
    }
}

std::vector<qint16> Metronome::renderClick(double freq, double durationSeconds, double decaySeconds, double amplitude)
{
    const int samples = static_cast<int>(SampleRate * durationSeconds);
    std::vector<qint16> click(samples);
    uint32_t seed = 1;
    for (int i = 0; i < samples; ++i) {
        const double t = static_cast<double>(i) / SampleRate;
        const double envelope = std::exp(-t / decaySeconds);
        seed = (seed * 1664525u) + 1013904223u;
        const double noise = (static_cast<double>(seed >> 8) / 8388608.0) - 1.0;
        const double tone = std::sin(2.0 * std::numbers::pi * freq * t);
        const double sample = (0.6 * tone) + (0.4 * noise);
        click[i] = static_cast<qint16>(sample * envelope * 32767.0 * amplitude);
    }
    return click;
}
