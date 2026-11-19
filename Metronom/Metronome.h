#pragma once

#include "ScreenLockInhibitor.h"
#include "TimingAnalyzer.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QList>
#include <QMediaDevices>
#include <QObject>
#include <QSpan>
#include <QString>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
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
    Q_PROPERTY(int numerator READ numerator WRITE setNumerator NOTIFY timeSignatureChanged)
    Q_PROPERTY(int denominator READ denominator WRITE setDenominator NOTIFY timeSignatureChanged)
    Q_PROPERTY(QList<int> numerators READ numerators CONSTANT)
    Q_PROPERTY(QList<int> denominators READ denominators CONSTANT)
    Q_PROPERTY(int beatsPerMeasure READ beatsPerMeasure NOTIFY timeSignatureChanged)
    Q_PROPERTY(NoteValue noteValue READ noteValue WRITE setNoteValue NOTIFY noteValueChanged)
    Q_PROPERTY(QList<int> availableNoteValues READ availableNoteValues NOTIFY timeSignatureChanged)
    Q_PROPERTY(int subdivisions READ subdivisions NOTIFY subdivisionsChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int currentBeat READ currentBeat NOTIFY ticked)
    Q_PROPERTY(int currentSubdivision READ currentSubdivision NOTIFY ticked)
    Q_PROPERTY(QList<int> tempoMarkings READ tempoMarkings CONSTANT)
    Q_PROPERTY(QString tempoMarking READ tempoMarking NOTIFY bpmChanged)
    Q_PROPERTY(QList<QString> audioDeviceNames READ audioDeviceNames NOTIFY audioDevicesChanged)
    Q_PROPERTY(int currentAudioDevice READ currentAudioDevice WRITE setCurrentAudioDevice NOTIFY
                   currentAudioDeviceChanged)
    Q_PROPERTY(bool autoIncrement READ autoIncrement WRITE setAutoIncrement NOTIFY autoIncrementChanged)
    Q_PROPERTY(int autoIncrementInterval READ autoIncrementInterval WRITE setAutoIncrementInterval NOTIFY
                   autoIncrementIntervalChanged)
    Q_PROPERTY(TimingAnalyzer *timing READ timing CONSTANT)

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

    int bpm() const;
    void setBpm(int bpm);
    int minBpm() const;
    int maxBpm() const;

    int numerator() const;
    void setNumerator(int numerator);
    int denominator() const;
    void setDenominator(int denominator);
    QList<int> numerators() const;
    QList<int> denominators() const;
    int beatsPerMeasure() const;

    NoteValue noteValue() const;
    void setNoteValue(NoteValue noteValue);
    QList<int> availableNoteValues() const;
    int subdivisions() const;

    bool running() const;
    int currentBeat() const;
    int currentSubdivision() const;
    QList<int> tempoMarkings() const;
    QString tempoMarking() const;

    QList<QString> audioDeviceNames() const;
    int currentAudioDevice() const;
    void setCurrentAudioDevice(int index);

    bool autoIncrement() const;
    void setAutoIncrement(bool enabled);
    int autoIncrementInterval() const;
    void setAutoIncrementInterval(int seconds);

    TimingAnalyzer *timing();

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void increaseBpm();
    Q_INVOKABLE void decreaseBpm();
    Q_INVOKABLE void nextTempoMarking();
    Q_INVOKABLE void previousTempoMarking();
    Q_INVOKABLE void refreshAudioDevices();

signals:
    void bpmChanged();
    void timeSignatureChanged();
    void noteValueChanged();
    void subdivisionsChanged();
    void runningChanged();
    void ticked();
    void currentAudioDeviceChanged();
    void audioDevicesChanged();
    void autoIncrementChanged();
    void autoIncrementIntervalChanged();

private:
    bool isCompound() const;
    int beatLength() const;
    int subdivisionsFor(NoteValue noteValue) const;
    void applyTimeSignature();
    void publishTickConfig();
    void pollPosition();

    template <typename T> void fillSamples(QSpan<T> samples) noexcept;
    template <typename T> void mixActiveClick(QSpan<T> samples, qint64 from, qint64 to) noexcept;
    void startTick() noexcept;
    void startBeat() noexcept;

    QAudioDevice deviceAt(int index) const;
    void applyPosition(std::uint32_t position);
    void rebuildAudio(const QAudioDevice &device);
    QAudioDevice calibrationOutput() const;
    bool useCalibrationOutput();
    void restartIfRunning();
    void updateAutoIncrementTimer();
    void finishCalibration();

    static constexpr int sampleRate = 44100;
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

    // Audio thread: active click and scheduler state.
    const std::vector<float> *m_activeClick = nullptr;
    std::size_t m_activeClickPos = 0;
    double m_nextTickFrame = 0.0;
    double m_beatStartFrame = 0.0;
    double m_beatFrames = 0.0;
    qint64 m_framesWritten = 0;

    std::unique_ptr<QAudioSink> m_sink;
    // Ticks waiting to be shown when they are heard.
    std::deque<std::pair<std::uint32_t, std::chrono::steady_clock::time_point>> m_heardPositions;
    std::chrono::steady_clock::duration m_outputDelay{};
    QByteArray m_sinkDeviceId;
    QList<QAudioDevice> m_devices;
    QMediaDevices m_mediaDevices;
    QAudioFormat m_format;
    std::vector<float> m_accentClick;
    std::vector<float> m_beatClick;
    std::vector<float> m_subdivisionClick;
    ScreenLockInhibitor m_screenLock;
    TimingAnalyzer m_timing;
    QTimer m_autoIncrementTimer;
    QTimer m_positionTimer;

    // Shared between threads: settings for the audio thread and the last tick it played.
    std::atomic<std::uint32_t> m_tickConfig{0};
    std::atomic<std::uint32_t> m_tickPosition{0};
    std::uint32_t m_lastTickPosition = 0;

    int m_audioBeatsPerMeasure = 1;
    int m_audioSubdivisions = 1;
    int m_audioBeat = 0;
    int m_audioSubdivision = 0;

    int m_currentDevice = 0;
    int m_sampleRate = sampleRate;
    int m_channelCount = 2;
    int m_bpm = 120;
    int m_numerator = 4;
    int m_denominator = 4;
    int m_currentBeat = 0;
    int m_currentSubdivision = 0;
    int m_autoIncrementInterval = 60;
    std::uint16_t m_tickSerial = 0;
    NoteValue m_noteValue = Quarter;
    bool m_running = false;
    bool m_autoIncrement = false;
    bool m_calibrating = false;
    bool m_calibratingOnOtherOutput = false;
    bool m_calibrationFailed = false;
};
