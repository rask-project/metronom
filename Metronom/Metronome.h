#pragma once

#include <QAudioDevice>
#include <QAudioFormat>
#include <QList>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <array>
#include <memory>
#include <vector>

class QAudioSink;

class Metronome : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int bpm READ bpm WRITE setBpm NOTIFY bpmChanged)
    Q_PROPERTY(int minBpm READ minBpm CONSTANT)
    Q_PROPERTY(int maxBpm READ maxBpm CONSTANT)
    Q_PROPERTY(NoteValue noteValue READ noteValue WRITE setNoteValue NOTIFY noteValueChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int currentBeat READ currentBeat NOTIFY currentBeatChanged)
    Q_PROPERTY(int beatsPerMeasure READ beatsPerMeasure NOTIFY noteValueChanged)
    Q_PROPERTY(QList<int> tempoMarkings READ tempoMarkings CONSTANT)
    Q_PROPERTY(QString tempoMarking READ tempoMarking NOTIFY bpmChanged)
    Q_PROPERTY(QList<QString> audioDeviceNames READ audioDeviceNames CONSTANT)
    Q_PROPERTY(int currentAudioDevice READ currentAudioDevice WRITE setCurrentAudioDevice NOTIFY
                   currentAudioDeviceChanged)

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
    int minBpm() const;
    int maxBpm() const;

    NoteValue noteValue() const;
    void setNoteValue(NoteValue noteValue);

    bool running() const;
    int currentBeat() const;
    int beatsPerMeasure() const;
    QList<int> tempoMarkings() const;
    QString tempoMarking() const;

    QList<QString> audioDeviceNames() const;
    int currentAudioDevice() const;
    void setCurrentAudioDevice(int index);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void increaseBpm();
    Q_INVOKABLE void decreaseBpm();
    Q_INVOKABLE void nextTempoMarking();
    Q_INVOKABLE void previousTempoMarking();

signals:
    void bpmChanged();
    void noteValueChanged();
    void runningChanged();
    void currentBeatChanged();
    void currentAudioDeviceChanged();

private:
    int subdivision() const;
    double tickIntervalSeconds() const;
    void resetScheduler();
    template <typename T> void fillSamplesT(T *data, int count);
    template <typename T> void writeClickT(T *data, int frameCount, int offset, const std::vector<float> &click);
    static std::vector<float>
    renderClick(double freq, double durationSeconds, double decaySeconds, double amplitude, int sampleRate);
    QAudioDevice deviceAt(int index) const;
    int defaultDeviceIndex() const;
    void rebuildAudio();

    static constexpr int sampleRate = 44100;
    static constexpr int beatsPerBar = 4;
    static constexpr std::array<int, 9> tempoMarkingsValues{40, 60, 66, 76, 108, 120, 168, 184, 208};
    static constexpr std::array<const char *, 9> tempoMarkingNames{
        "Largo",
        "Larghetto",
        "Adagio",
        "Andante",
        "Moderato",
        "Allegro",
        "Vivace",
        "Presto",
        "Prestissimo"
    };

    QList<QAudioDevice> m_devices;
    int m_currentDevice = 0;
    QAudioFormat m_format;
    int m_sampleRate = sampleRate;
    int m_channelCount = 2;
    std::vector<float> m_strongClick;
    std::vector<float> m_weakClick;
    std::vector<float> m_pendingTail;
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
