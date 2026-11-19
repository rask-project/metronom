#pragma once

#include "ScreenLockInhibitor.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QList>
#include <QObject>
#include <QSpan>
#include <QString>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>
#include <array>
#include <atomic>
#include <memory>

class QAudioSink;

class Metronome : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint32 bpm READ bpm WRITE setBpm NOTIFY bpmChanged)
    Q_PROPERTY(qint32 minBpm READ minBpm CONSTANT)
    Q_PROPERTY(qint32 maxBpm READ maxBpm CONSTANT)
    Q_PROPERTY(qint32 numerator READ numerator WRITE setNumerator NOTIFY timeSignatureChanged)
    Q_PROPERTY(qint32 denominator READ denominator WRITE setDenominator NOTIFY timeSignatureChanged)
    Q_PROPERTY(QList<qint32> numerators READ numerators CONSTANT)
    Q_PROPERTY(QList<qint32> denominators READ denominators CONSTANT)
    Q_PROPERTY(qint32 beatsPerMeasure READ beatsPerMeasure NOTIFY timeSignatureChanged)
    Q_PROPERTY(NoteValue noteValue READ noteValue WRITE setNoteValue NOTIFY noteValueChanged)
    Q_PROPERTY(QList<qint32> availableNoteValues READ availableNoteValues NOTIFY timeSignatureChanged)
    Q_PROPERTY(qint32 subdivisions READ subdivisions NOTIFY subdivisionsChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(qint32 currentBeat READ currentBeat NOTIFY ticked)
    Q_PROPERTY(qint32 currentSubdivision READ currentSubdivision NOTIFY ticked)
    Q_PROPERTY(QList<qint32> tempoMarkings READ tempoMarkings CONSTANT)
    Q_PROPERTY(QString tempoMarking READ tempoMarking NOTIFY bpmChanged)
    Q_PROPERTY(QList<QString> audioDeviceNames READ audioDeviceNames CONSTANT)
    Q_PROPERTY(qint32 currentAudioDevice READ currentAudioDevice WRITE setCurrentAudioDevice NOTIFY
                   currentAudioDeviceChanged)
    Q_PROPERTY(bool autoIncrement READ autoIncrement WRITE setAutoIncrement NOTIFY autoIncrementChanged)
    Q_PROPERTY(qint32 autoIncrementInterval READ autoIncrementInterval WRITE setAutoIncrementInterval NOTIFY
                   autoIncrementIntervalChanged)

public:
    // Values are persisted in the settings: append new ones, never reorder.
    enum NoteValue : quint8 {
        Quarter = 0,
        Eighth,
        Sixteenth,
        ThirtySecond,
        Half,
        DottedQuarter
    };
    Q_ENUM(NoteValue)

    explicit Metronome(QObject *parent = nullptr);
    ~Metronome() override;

    qint32 bpm() const;
    void setBpm(qint32 bpm);
    qint32 minBpm() const;
    qint32 maxBpm() const;

    qint32 numerator() const;
    void setNumerator(qint32 numerator);
    qint32 denominator() const;
    void setDenominator(qint32 denominator);
    QList<qint32> numerators() const;
    QList<qint32> denominators() const;
    qint32 beatsPerMeasure() const;

    NoteValue noteValue() const;
    void setNoteValue(NoteValue noteValue);
    QList<qint32> availableNoteValues() const;
    qint32 subdivisions() const;

    bool running() const;
    qint32 currentBeat() const;
    qint32 currentSubdivision() const;
    QList<qint32> tempoMarkings() const;
    QString tempoMarking() const;

    QList<QString> audioDeviceNames() const;
    qint32 currentAudioDevice() const;
    void setCurrentAudioDevice(qint32 index);

    bool autoIncrement() const;
    void setAutoIncrement(bool enabled);
    qint32 autoIncrementInterval() const;
    void setAutoIncrementInterval(qint32 seconds);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void increaseBpm();
    Q_INVOKABLE void decreaseBpm();
    Q_INVOKABLE void nextTempoMarking();
    Q_INVOKABLE void previousTempoMarking();

signals:
    void bpmChanged();
    void timeSignatureChanged();
    void noteValueChanged();
    void subdivisionsChanged();
    void runningChanged();
    void ticked();
    void currentAudioDeviceChanged();
    void autoIncrementChanged();
    void autoIncrementIntervalChanged();

private:
    bool isCompound() const;
    qint32 beatLength() const;
    qint32 subdivisionsFor(NoteValue noteValue) const;
    void applyTimeSignature();
    void publishTickConfig();
    void pollPosition();

    template <typename T> void fillSamples(QSpan<T> samples) noexcept;
    template <typename T> void mixActiveClick(QSpan<T> samples, qsizetype from, qsizetype to) noexcept;
    void startTick() noexcept;
    void startBeat() noexcept;

    QAudioDevice deviceAt(qint32 index) const;
    qint32 defaultDeviceIndex() const;
    void rebuildAudio();
    void updateAutoIncrementTimer();

    static constexpr qint32 sampleRate{44100};
    static constexpr std::array<qint32, 9> tempoMarkingsValues{40, 60, 66, 76, 108, 120, 168, 184, 208};
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

    // Audio thread: active click and scheduler state.
    const QList<float> *m_activeClick{nullptr};
    qsizetype m_activeClickPos{0};
    double m_nextTickFrame{0.0};
    double m_beatStartFrame{0.0};
    double m_beatFrames{0.0};
    qint64 m_framesWritten{0};

    std::unique_ptr<QAudioSink> m_sink;
    QList<QAudioDevice> m_devices;
    QAudioFormat m_format;
    QList<float> m_accentClick;
    QList<float> m_beatClick;
    QList<float> m_subdivisionClick;
    ScreenLockInhibitor m_screenLock;
    QTimer m_autoIncrementTimer;
    QTimer m_positionTimer;

    // Shared between threads: settings for the audio thread and the last tick it played.
    std::atomic<quint32> m_tickConfig{0};
    std::atomic<quint32> m_tickPosition{0};
    quint32 m_lastTickPosition{0};

    qint32 m_audioBeatsPerMeasure{1};
    qint32 m_audioSubdivisions{1};
    qint32 m_audioBeat{0};
    qint32 m_audioSubdivision{0};

    qint32 m_currentDevice{0};
    qint32 m_sampleRate{sampleRate};
    qint32 m_channelCount{2};
    qint32 m_bpm{120};
    qint32 m_numerator{4};
    qint32 m_denominator{4};
    qint32 m_currentBeat{0};
    qint32 m_currentSubdivision{0};
    qint32 m_autoIncrementInterval{60};
    quint16 m_tickSerial{0};
    NoteValue m_noteValue{Quarter};
    bool m_running{false};
    bool m_autoIncrement{false};
};
