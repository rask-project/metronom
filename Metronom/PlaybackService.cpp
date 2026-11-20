#include "PlaybackService.h"

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/private/qjnihelpers_p.h>

namespace
{
constexpr const char *serviceClass{"io/github/rask_project/metronom/MetronomeService"};

struct Callbacks {
    std::function<void()> play;
    std::function<void()> stop;
};

Callbacks &callbacks()
{
    static Callbacks instance;
    return instance;
}

// Notification actions arrive on the Android UI thread.
void invoke(std::function<void()> Callbacks::*callback)
{
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [callback] {
            const std::function<void()> &function{callbacks().*callback};
            if (function != nullptr) {
                function();
            }
        },
        Qt::QueuedConnection
    );
}

void nativePlay(JNIEnv * /*env*/, jclass /*clazz*/)
{
    invoke(&Callbacks::play);
}

void nativeStop(JNIEnv * /*env*/, jclass /*clazz*/)
{
    invoke(&Callbacks::stop);
}
} // namespace

Q_DECLARE_JNI_NATIVE_METHOD(nativePlay)
Q_DECLARE_JNI_NATIVE_METHOD(nativeStop)
#endif

PlaybackService::PlaybackService(
    [[maybe_unused]] const std::function<void()> &onPlayRequested,
    [[maybe_unused]] const std::function<void()> &onStopRequested
)
{
#ifdef Q_OS_ANDROID
    callbacks() = {.play = onPlayRequested, .stop = onStopRequested};
    QJniEnvironment env;
    env.registerNativeMethods(serviceClass, {Q_JNI_NATIVE_METHOD(nativePlay), Q_JNI_NATIVE_METHOD(nativeStop)});
#endif
}

PlaybackService::~PlaybackService()
{
#ifdef Q_OS_ANDROID
    callbacks() = {};
    if (m_shown) {
        const QJniObject context{QNativeInterface::QAndroidApplication::context()};
        QJniObject::callStaticMethod<void>(serviceClass, "hide", "(Landroid/content/Context;)V", context.object());
    }
#endif
}

void PlaybackService::setPlaying(bool playing)
{
    if (m_playing == playing) {
        return;
    }
    m_playing = playing;
    // The notification appears with the first play and then stays, paused or playing.
    if (!playing && !m_shown) {
        return;
    }

#ifdef Q_OS_ANDROID
    if (!m_shown) {
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
            const QJniObject activity{QtAndroidPrivate::activity()};
            if (activity.isValid()) {
                QJniObject::callStaticMethod<void>(
                    serviceClass,
                    "requestNotificationPermission",
                    "(Landroid/app/Activity;)V",
                    activity.object()
                );
            }
        });
    }
#endif
    m_shown = true;
#ifdef Q_OS_ANDROID
    show();
#endif
}

void PlaybackService::setContent(
    const QString &text,
    qint32 noteValue,
    const QString &playText,
    const QString &stopText
)
{
    if (text == m_text && noteValue == m_noteValue && playText == m_playText && stopText == m_stopText) {
        return;
    }
    m_text = text;
    m_noteValue = noteValue;
    m_playText = playText;
    m_stopText = stopText;

#ifdef Q_OS_ANDROID
    if (m_shown) {
        show();
    }
#endif
}

#ifdef Q_OS_ANDROID
void PlaybackService::show() const
{
    const QJniObject context{QNativeInterface::QAndroidApplication::context()};
    QJniObject::callStaticMethod<void>(
        serviceClass,
        "show",
        "(Landroid/content/Context;ZLjava/lang/String;ILjava/lang/String;Ljava/lang/String;)V",
        context.object(),
        static_cast<jboolean>(m_playing),
        QJniObject::fromString(m_text).object<jstring>(),
        static_cast<jint>(m_noteValue),
        QJniObject::fromString(m_playText).object<jstring>(),
        QJniObject::fromString(m_stopText).object<jstring>()
    );
}
#endif
