#pragma once

#include "SpscQueue.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSpan>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <vector>

class QAudioSource;

// Maps a stream's frame count to the steady clock, estimated from the times its audio callbacks run.
class StreamClock
{
public:
    void reset(int sampleRate) noexcept;
    void update(qint64 frame) noexcept;
    std::optional<double> timeOf(qint64 frame) const noexcept;

private:
    std::atomic<double> m_offset{0.0};
    double m_estimate = 0.0;
    double m_sampleRate = 1.0;
    std::atomic<bool> m_valid{false};
};

// Listens to the microphone while the metronome plays and compares the hits with the clicks.
class TimingAnalyzer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by Metronome.timing")

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool calibrated READ calibrated NOTIFY calibratedChanged)
    Q_PROPERTY(QList<QString> inputDeviceNames READ inputDeviceNames NOTIFY inputDevicesChanged)
    Q_PROPERTY(int currentInputDevice READ currentInputDevice WRITE setCurrentInputDevice NOTIFY
                   currentInputDeviceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool hasHit READ hasHit NOTIFY hit)
    Q_PROPERTY(int lastOffset READ lastOffset NOTIFY hit)
    Q_PROPERTY(Accuracy lastAccuracy READ lastAccuracy NOTIFY hit)
    Q_PROPERTY(int playedBpm READ playedBpm NOTIFY playedBpmChanged)
    // One entry per click of the measure last played: its Accuracy, or -1 without a hit.
    Q_PROPERTY(QList<int> measureHits READ measureHits NOTIFY hit)

public:
    enum Status : quint8 {
        Idle,
        Calibrating,
        Listening,
        CalibrationFailed,
        MicrophoneUnavailable
    };
    Q_ENUM(Status)

    enum Accuracy : quint8 {
        OnTime,
        Early,
        Late
    };
    Q_ENUM(Accuracy)

    // A click as the output thread plays it.
    struct Tick {
        qint64 frame;
        int bpm;
        int beatsPerMeasure;
        int subdivisions;
        int beat;
        int subdivision;
    };

    explicit TimingAnalyzer(QObject *parent = nullptr);
    ~TimingAnalyzer() override;

    bool enabled() const;
    void setEnabled(bool enabled);
    bool calibrated() const;
    QList<QString> inputDeviceNames() const;
    int currentInputDevice() const;
    void setCurrentInputDevice(int index);
    void refreshInputDevices();
    // Each output and input pair has its own calibration.
    void setOutputDevice(const QByteArray &id);
    Status status() const;
    bool hasHit() const;
    int lastOffset() const;
    Accuracy lastAccuracy() const;
    int playedBpm() const;
    QList<int> measureHits() const;

    Q_INVOKABLE void recalibrate();

    bool needsCalibration() const;
    // Added to the next calibration, which measures another output than the chosen one.
    void setLatencyAdjustment(double seconds);
    // Returns true when this run starts with the calibration clicks (only if calibrate allows them).
    bool start(int outputSampleRate, bool calibrate);
    void stop();

    // Output audio thread.
    void outputCallback(qint64 framesWritten) noexcept;
    void tickPlayed(const Tick &tick) noexcept;

signals:
    void enabledChanged();
    void calibratedChanged();
    void currentInputDeviceChanged();
    void inputDevicesChanged();
    // The microphone in use changed, so a running analysis has to start again.
    void inputDeviceSwitched();
    void statusChanged();
    void hit();
    void playedBpmChanged();
    void calibrationFinished();
    void microphoneGranted();

private:
    struct TickEvent {
        qint64 frame;
        qint64 index;   // set when drained
        qint64 measure; // set when drained
        std::uint16_t bpm;
        std::uint8_t beatsPerMeasure;
        std::uint8_t subdivisions;
        std::uint8_t beat;
        std::uint8_t subdivision;
    };

    struct TickMatch {
        double offset;
        double tickTime;
        double tickSeconds;
        TickEvent tick;
    };

    struct Hit {
        TickMatch match;
        double time;
    };

    // The hits around one click, until the next click: the first one that is surely the player is shown at once;
    // one right on the click may be the metronome's own echo, which is only known when the click's window closes.
    struct OpenTick {
        std::optional<Hit> nearClick;
        std::optional<Hit> apart;
        double closesAt;
        qint64 index;
        bool shown;
    };

    struct Interval {
        double seconds;
        double beats;
    };

    template <typename T> void processInput(QSpan<const T> samples) noexcept;
    void finishBlock(qint64 blockStart) noexcept;
    qint64 attackBlocks() const noexcept;

    QString calibrationKey() const;
    void loadCalibration();
    void requestMicrophone();
    bool startSource();
    void setStatus(Status status);
    void resetResults();
    void clearResult();
    void clearStaleHit();
    double maxHitGap() const;
    void process();
    void drainEvents();
    void processCalibration();
    void finishCalibration();
    void matchOnsets();
    std::optional<TickMatch> nearestTick(double time) const;
    std::optional<double> latestTickBefore(double time) const;
    void collectHit(const Hit &hit);
    void resolveOpenTick();
    void recordSilentClicks(qint64 tickIndex);
    void registerHit(const Hit &detected);
    void markMeasureHit(const TickEvent &tick);
    void updatePlayedBpm(const Hit &hit);

    std::unique_ptr<QAudioSource> m_source;

    // Input audio thread: onset detector.
    double m_blockEnergy = 0.0;
    double m_background = 0.0;
    std::array<double, 12> m_recentEnergy{};
    std::size_t m_recentIndex = 0;
    qint64 m_inputFrames = 0;
    qint64 m_warmupFrames = 0;
    int m_blockFrames = 1;
    int m_blockFill = 0;
    int m_blocksSinceOnset = 0;
    int m_refractoryBlocks = 1;
    int m_inputChannels = 1;
    float m_previousSample = 0.0f;
    bool m_armed = true;

    std::optional<OpenTick> m_openTick;
    std::optional<double> m_lastHitTime;
    double m_latency = 0.0;
    double m_latencyAdjustment = 0.0;
    qint64 m_tickCount = 0;
    qint64 m_measure = 0;
    qint64 m_hitsMeasure = -1;
    qint64 m_lastResolvedTick = -1;
    QList<QAudioDevice> m_inputDevices;
    QList<int> m_measureHits;
    QByteArray m_outputId;
    std::deque<TickEvent> m_ticks;
    std::deque<qint64> m_pendingOnsets;
    std::deque<Interval> m_intervals;
    std::vector<double> m_calibrationLatencies;
    QAudioFormat m_inputFormat;
    QTimer m_processTimer;
    SpscQueue<TickEvent, 256> m_tickQueue;
    SpscQueue<qint64, 256> m_onsetQueue;
    StreamClock m_outputClock;
    StreamClock m_inputClock;

    int m_currentInputDevice = 0;
    int m_lastOffset = 0;
    int m_playedBpm = 0;
    // One bit per recent click with no hit apart from it: whether the microphone heard something right on it.
    std::uint8_t m_echoes = 0xFF;
    std::atomic<bool> m_active{false};
    Status m_status = Idle;
    Accuracy m_lastAccuracy = OnTime;
    bool m_enabled = false;
    bool m_calibrated = false;
    bool m_hasHit = false;
};
