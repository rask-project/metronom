#include "AppInfo.h"

AppInfo::AppInfo(QObject *parent) : QObject(parent) {}

QString AppInfo::qtVersion()
{
    return QString::fromLatin1(qVersion());
}

QString AppInfo::qtBuildVersion()
{
    return QStringLiteral(QT_VERSION_STR);
}

bool AppInfo::nativeMenuBar() noexcept
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}
