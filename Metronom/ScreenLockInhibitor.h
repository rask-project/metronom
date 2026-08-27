#pragma once

#include <QtGlobal>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <cstdint>
#include <memory>
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
struct wl_compositor;
struct wl_display;
struct wl_registry;
struct wl_registry_listener;
struct wl_shm;
struct wl_surface;
struct zwp_idle_inhibit_manager_v1;
struct zwp_idle_inhibitor_v1;
#endif

class ScreenLockInhibitor
{
public:
    ScreenLockInhibitor();
    ~ScreenLockInhibitor();

    void setInhibited(bool inhibited);

private:
#ifdef Q_OS_MACOS
    uint32_t m_assertionId = 0;
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    static void disconnectDisplay(struct wl_display *display);
    static void destroyManager(struct zwp_idle_inhibit_manager_v1 *manager);
    static void destroyInhibitor(struct zwp_idle_inhibitor_v1 *inhibitor);
    static void destroySurface(struct wl_surface *surface);
    static void
    registryGlobal(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

    std::unique_ptr<struct wl_registry_listener> m_registryListener;
    std::unique_ptr<struct wl_display, void (*)(struct wl_display *)> m_waylandDisplay;
    std::unique_ptr<struct zwp_idle_inhibit_manager_v1, void (*)(struct zwp_idle_inhibit_manager_v1 *)> m_manager;
    std::unique_ptr<struct wl_surface, void (*)(struct wl_surface *)> m_surface;
    std::unique_ptr<struct zwp_idle_inhibitor_v1, void (*)(struct zwp_idle_inhibitor_v1 *)> m_inhibitor;
    struct wl_compositor *m_compositor = nullptr;
    struct wl_shm *m_shm = nullptr;

    bool establishConnection();
    void releaseWayland();
    void setX11Inhibited(bool inhibited);
    void setWaylandInhibited(bool inhibited);
    void setDbusInhibited(bool inhibited);

    uint32_t m_powerManagementCookie = 0;
    uint32_t m_screenSaverCookie = 0;
#endif
};
