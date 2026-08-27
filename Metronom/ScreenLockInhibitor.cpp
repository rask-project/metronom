#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#define _GNU_SOURCE
#endif

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
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QGuiApplication>
#include <QWindow>

#include <xcb/xcb.h>

#include <wayland-client.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "idle-inhibit-unstable-v1-protocol.h"
#endif

ScreenLockInhibitor::ScreenLockInhibitor()
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    : m_waylandDisplay(nullptr, &ScreenLockInhibitor::disconnectDisplay)
    , m_manager(nullptr, &ScreenLockInhibitor::destroyManager)
    , m_surface(nullptr, &ScreenLockInhibitor::destroySurface)
    , m_inhibitor(nullptr, &ScreenLockInhibitor::destroyInhibitor)
#endif
{}

ScreenLockInhibitor::~ScreenLockInhibitor()
{
    setInhibited(false);
}

void ScreenLockInhibitor::setInhibited(bool inhibited)
{
    qDebug() << "ScreenLockInhibitor::setInhibited" << inhibited;

#ifdef Q_OS_WIN
    SetThreadExecutionState(inhibited ? (ES_CONTINUOUS | ES_DISPLAY_REQUIRED) : ES_CONTINUOUS);
#elifdef Q_OS_MACOS
    if (inhibited) {
        if (m_assertionId == 0) {
            IOPMAssertionID assertionId = 0;
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
        const QJniObject activity = QtAndroidPrivate::activity();
        const QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) {
            return;
        }
        const jint keepScreenOn =
            QJniObject::getStaticField<jint>("android/view/WindowManager$LayoutParams", "FLAG_KEEP_SCREEN_ON");
        window.callMethod(inhibited ? "addFlags" : "clearFlags", "(I)V", keepScreenOn);
    });
#else
    setDbusInhibited(inhibited);
    if (QGuiApplication::platformName() == QLatin1String("xcb")) {
        setX11Inhibited(inhibited);
    } else {
        setWaylandInhibited(inhibited);
    }
#endif
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

void ScreenLockInhibitor::disconnectDisplay(struct wl_display *display)
{
    wl_display_disconnect(display);
}

void ScreenLockInhibitor::destroyManager(struct zwp_idle_inhibit_manager_v1 *manager)
{
    zwp_idle_inhibit_manager_v1_destroy(manager);
}

void ScreenLockInhibitor::destroyInhibitor(struct zwp_idle_inhibitor_v1 *inhibitor)
{
    zwp_idle_inhibitor_v1_destroy(inhibitor);
}

void ScreenLockInhibitor::destroySurface(struct wl_surface *surface)
{
    wl_surface_destroy(surface);
}

void ScreenLockInhibitor::registryGlobal(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
)
{
    auto *self = static_cast<ScreenLockInhibitor *>(data);
    if (version < 1) {
        return;
    }
    if (std::strcmp(interface, "zwp_idle_inhibit_manager_v1") == 0) {
        if (self->m_manager == nullptr) {
            self->m_manager.reset(static_cast<struct zwp_idle_inhibit_manager_v1 *>(
                wl_registry_bind(registry, name, &zwp_idle_inhibit_manager_v1_interface, version)
            ));
        }
    } else if (std::strcmp(interface, "wl_compositor") == 0) {
        if (self->m_compositor == nullptr) {
            self->m_compositor =
                static_cast<struct wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, version)
                );
        }
    } else if (std::strcmp(interface, "wl_shm") == 0) {
        if (self->m_shm == nullptr) {
            self->m_shm = static_cast<struct wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, version));
        }
    }
}

bool ScreenLockInhibitor::establishConnection()
{
    m_waylandDisplay.reset(wl_display_connect(nullptr));
    if (m_waylandDisplay == nullptr) {
        return false;
    }
    struct wl_registry *registry = wl_display_get_registry(m_waylandDisplay.get());
    m_registryListener = std::make_unique<struct wl_registry_listener>();
    m_registryListener->global = &ScreenLockInhibitor::registryGlobal;
    wl_registry_add_listener(registry, m_registryListener.get(), this);
    wl_display_roundtrip(m_waylandDisplay.get());
    if (m_manager == nullptr || m_compositor == nullptr || m_shm == nullptr) {
        releaseWayland();
        return false;
    }

    m_surface.reset(wl_compositor_create_surface(m_compositor));
    const int fd = memfd_create("metronom-idle-inhibit", 0);
    if (fd < 0) {
        releaseWayland();
        return false;
    }
    ftruncate(fd, sizeof(uint32_t));
    struct wl_shm_pool *pool = wl_shm_create_pool(m_shm, fd, static_cast<int32_t>(sizeof(uint32_t)));
    wl_display_flush(m_waylandDisplay.get());
    close(fd);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, 1, 1, 4, WL_SHM_FORMAT_ARGB8888);
    wl_surface_attach(m_surface.get(), buffer, 0, 0);
    wl_surface_commit(m_surface.get());
    wl_buffer_destroy(buffer);
    wl_shm_pool_destroy(pool);
    return true;
}

void ScreenLockInhibitor::releaseWayland()
{
    if (m_waylandDisplay == nullptr) {
        return;
    }
    m_inhibitor.reset();
    m_surface.reset();
    m_manager.reset();
    m_compositor = nullptr;
    m_shm = nullptr;
    wl_display_flush(m_waylandDisplay.get());
    m_waylandDisplay.reset();
    m_registryListener.reset();
}

void ScreenLockInhibitor::setX11Inhibited(bool inhibited)
{
    const QWindow *window = QGuiApplication::topLevelWindows().first();
    if (window == nullptr) {
        return;
    }

    xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
    if (connection == nullptr || xcb_connection_has_error(connection) != 0) {
        return;
    }

    const xcb_window_t xid = static_cast<xcb_window_t>(window->winId());
    const uint32_t value = inhibited ? 1 : 0;
    for (const char *name : {"_NET_WM_INHIBIT_CLIENT", "_NET_WM_INHIBIT_SCREEN"}) {
        const xcb_intern_atom_cookie_t cookie =
            xcb_intern_atom(connection, 0, static_cast<uint8_t>(std::strlen(name)), name);
        std::unique_ptr<xcb_intern_atom_reply_t, decltype(&free)> reply(
            xcb_intern_atom_reply(connection, cookie, nullptr),
            &free);
        if (reply != nullptr) {
            xcb_change_property(connection, XCB_PROP_MODE_REPLACE, xid, reply->atom, XCB_ATOM_CARDINAL, 32, 1, &value);
        }
    }
    xcb_flush(connection);
    xcb_disconnect(connection);
}

void ScreenLockInhibitor::setWaylandInhibited(bool inhibited)
{
    if (!inhibited) {
        releaseWayland();
        return;
    }
    if (m_inhibitor != nullptr) {
        return;
    }
    if (!establishConnection()) {
        return;
    }
    m_inhibitor.reset(zwp_idle_inhibit_manager_v1_create_inhibitor(m_manager.get(), m_surface.get()));
    wl_display_flush(m_waylandDisplay.get());
}

namespace {
uint32_t callInhibit(const char *service, const char *path, const char *interface, const char *method)
{
    const QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
        return 0;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments({"Metronom", "Metronome is playing"});
    const QDBusReply<uint32_t> reply = connection.call(message, QDBus::Block, 2000);
    return reply.isValid() ? reply.value() : 0;
}

void callUninhibit(const char *service, const char *path, const char *interface, const char *method, uint32_t cookie)
{
    const QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected() || cookie == 0) {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments({cookie});
    connection.call(message, QDBus::Block, 2000);
}
} // namespace

void ScreenLockInhibitor::setDbusInhibited(bool inhibited)
{
    if (inhibited) {
        m_powerManagementCookie = callInhibit("org.freedesktop.PowerManagement", "/org/freedesktop/PowerManagement/Inhibit",
                                              "org.freedesktop.PowerManagement.Inhibit", "Inhibit");
        m_screenSaverCookie = callInhibit("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", "Inhibit");
    } else {
        callUninhibit("org.freedesktop.PowerManagement", "/org/freedesktop/PowerManagement/Inhibit",
                      "org.freedesktop.PowerManagement.Inhibit", "UnInhibit", m_powerManagementCookie);
        m_powerManagementCookie = 0;
        callUninhibit("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", "UnInhibit",
                      m_screenSaverCookie);
        m_screenSaverCookie = 0;
    }
}

#endif
