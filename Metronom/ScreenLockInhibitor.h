#pragma once

#include <QtGlobal>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <QString>
#endif

class ScreenLockInhibitor
{
public:
    ScreenLockInhibitor() = default;
    ~ScreenLockInhibitor();

    ScreenLockInhibitor(const ScreenLockInhibitor &) = delete;
    ScreenLockInhibitor &operator=(const ScreenLockInhibitor &) = delete;

    void setInhibited(bool inhibited);

private:
#ifdef Q_OS_MACOS
    quint32 m_assertionId = 0;
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    void setScreenSaverInhibited(bool inhibited);
    void setPortalInhibited(bool inhibited);

    QString m_portalRequest;
    quint32 m_screenSaverCookie = 0;
#endif
    bool m_inhibited = false;
};
