#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>
#include <memory>
#include <vector>

class QAudioSink;

class Metronome : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int bpm READ bpm WRITE setBpm NOTIFY bpmChanged)
    Q_PROPERTY(NoteValue noteValue READ noteValue WRITE setNoteValue NOTIFY noteValueChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int currentBeat READ currentBeat NOTIFY currentBeatChanged)
    Q_PROPERTY(int beatsPerMeasure READ beatsPerMeasure NOTIFY noteValueChanged)

public:
    enum NoteValue : quint8 {
        Quarter = 0,
        Eighth,
        Sixteenth,
        ThirtySecond
    };
    Q_ENUM(NoteValue)

    explicit Metronome(QObject *parent = nullptr);
    ~Metronome() override;

    int bpm() const;
    void setBpm(int bpm);

    NoteValue noteValue() const;
    void setNoteValue(NoteValue noteValue);

    bool running() const;
    int currentBeat() const;
    int beatsPerMeasure() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void increaseBpm();
    Q_INVOKABLE void decreaseBpm();

signals:
    void bpmChanged();
    void noteValueChanged();
    void runningChanged();
    void currentBeatChanged();

private:
    int subdivision() const;
    double tickIntervalSeconds() const;
    void resetScheduler();
    void fillSamples(qint16 *data, int count);
    void writeClick(qint16 *data, int frameCount, int offset, const std::vector<qint16> &click);
    static std::vector<qint16> renderClick(double freq, double durationSeconds, double decaySeconds, double amplitude);

    static constexpr int MinBpm = 20;
    static constexpr int MaxBpm = 300;
    static constexpr int SampleRate = 44100;
    static constexpr int BeatsPerBar = 4;

    std::vector<qint16> m_strongClick;
    std::vector<qint16> m_weakClick;
    std::vector<qint16> m_pendingTail;
    std::unique_ptr<QAudioSink> m_sink;
    double m_nextTickSample = 0.0;
    double m_tickIntervalSamples = 0.0;
    qint64 m_framesWritten = 0;
    int m_bpm = 120;
    int m_currentBeat = 0;
    int m_beatIndex = 0;
    NoteValue m_noteValue = Quarter;
    bool m_running = false;
};
