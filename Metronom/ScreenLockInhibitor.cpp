#include "ScreenLockInhibitor.h"

#ifdef Q_OS_WIN
#include <windows.h>
#elifdef Q_OS_MACOS
#include <IOKit/pwr_mgt/IOPMLib.h>
#elifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#include <QtCore/private/qjnihelpers_p.h>
#else
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QVariantMap>
#endif

ScreenLockInhibitor::~ScreenLockInhibitor()
{
    setInhibited(false);
}

void ScreenLockInhibitor::setInhibited(bool inhibited)
{
    if (m_inhibited == inhibited) {
        return;
    }
    m_inhibited = inhibited;

#ifdef Q_OS_WIN
    SetThreadExecutionState(inhibited ? (ES_CONTINUOUS | ES_DISPLAY_REQUIRED) : ES_CONTINUOUS);
#elifdef Q_OS_MACOS
    if (inhibited) {
        if (m_assertionId == 0) {
            IOPMAssertionID assertionId{0};
            if (IOPMAssertionCreateWithName(
                    kIOPMAssertPreventUserIdleDisplaySleep,
                    kIOPMAssertionLevelOn,
                    CFSTR("Metronom is running"),
                    &assertionId
                ) == kIOReturnSuccess) {
                m_assertionId = assertionId;
            }
        }
    } else if (m_assertionId != 0) {
        IOPMAssertionRelease(m_assertionId);
        m_assertionId = 0;
    }
#elifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([inhibited]() {
        const QJniObject activity{QtAndroidPrivate::activity()};
        const QJniObject window{activity.callObjectMethod("getWindow", "()Landroid/view/Window;")};
        if (!window.isValid()) {
            return;
        }
        const jint keepScreenOn{
            QJniObject::getStaticField<jint>("android/view/WindowManager$LayoutParams", "FLAG_KEEP_SCREEN_ON")
        };
        window.callMethod(inhibited ? "addFlags" : "clearFlags", "(I)V", keepScreenOn);
    });
#else
    setScreenSaverInhibited(inhibited);
    setPortalInhibited(inhibited);
#endif
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

namespace
{
constexpr qint32 dbusTimeoutMs{2000};
constexpr quint32 portalInhibitIdle{8};
constexpr const char *reason{"Metronome is playing"};
} // namespace

// KDE Plasma and most X11 desktops.
void ScreenLockInhibitor::setScreenSaverInhibited(bool inhibited)
{
    const QDBusConnection bus{QDBusConnection::sessionBus()};
    if (!bus.isConnected()) {
        return;
    }

    if (inhibited) {
        QDBusMessage message{QDBusMessage::createMethodCall(
            "org.freedesktop.ScreenSaver",
            "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver",
            "Inhibit"
        )};
        message.setArguments({QCoreApplication::applicationName(), QString::fromLatin1(reason)});
        const QDBusReply<quint32> reply{bus.call(message, QDBus::Block, dbusTimeoutMs)};
        m_screenSaverCookie = reply.isValid() ? reply.value() : 0;
        return;
    }

    if (m_screenSaverCookie == 0) {
        return;
    }
    QDBusMessage message{QDBusMessage::createMethodCall(
        "org.freedesktop.ScreenSaver",
        "/org/freedesktop/ScreenSaver",
        "org.freedesktop.ScreenSaver",
        "UnInhibit"
    )};
    message.setArguments({m_screenSaverCookie});
    bus.call(message, QDBus::Block, dbusTimeoutMs);
    m_screenSaverCookie = 0;
}

// GNOME, Flatpak and any desktop with xdg-desktop-portal.
void ScreenLockInhibitor::setPortalInhibited(bool inhibited)
{
    const QDBusConnection bus{QDBusConnection::sessionBus()};
    if (!bus.isConnected()) {
        return;
    }

    if (inhibited) {
        QDBusMessage message{QDBusMessage::createMethodCall(
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Inhibit",
            "Inhibit"
        )};
        message.setArguments(
            {QString(), portalInhibitIdle, QVariantMap{{QStringLiteral("reason"), QString::fromLatin1(reason)}}}
        );
        const QDBusReply<QDBusObjectPath> reply{bus.call(message, QDBus::Block, dbusTimeoutMs)};
        m_portalRequest = reply.isValid() ? reply.value().path() : QString();
        return;
    }

    if (m_portalRequest.isEmpty()) {
        return;
    }
    const QDBusMessage message{QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        m_portalRequest,
        "org.freedesktop.portal.Request",
        "Close"
    )};
    bus.call(message, QDBus::Block, dbusTimeoutMs);
    m_portalRequest.clear();
}

#endif
