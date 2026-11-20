#pragma once

#include <QString>
#include <functional>

// Android: a foreground service whose notification keeps the metronome playing in the background and offers
// stop/resume. No-op on the other platforms.
class PlaybackService
{
public:
    PlaybackService(const std::function<void()> &onPlayRequested, const std::function<void()> &onStopRequested);
    ~PlaybackService();

    PlaybackService(const PlaybackService &) = delete;
    PlaybackService &operator=(const PlaybackService &) = delete;

    void setPlaying(bool playing);
    void setContent(const QString &text, qint32 noteValue, const QString &playText, const QString &stopText);

private:
#ifdef Q_OS_ANDROID
    void show() const;
#endif

    QString m_text;
    QString m_playText;
    QString m_stopText;
    qint32 m_noteValue{0};
    bool m_playing{false};
    bool m_shown{false};
};
