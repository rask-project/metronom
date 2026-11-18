#include "AppInfo.h"

#include <QLocale>

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

QUrl AppInfo::websiteUrl()
{
    static constexpr auto site = "https://rask-project.github.io/metronom/";
    // QLocale() is the language main.cpp loaded the translation for.
    switch (QLocale().language()) {
    case QLocale::Portuguese:
        return QUrl(QString::fromLatin1(site) + QStringLiteral("pt-br/"));
    case QLocale::Spanish:
        return QUrl(QString::fromLatin1(site) + QStringLiteral("es/"));
    case QLocale::French:
        return QUrl(QString::fromLatin1(site) + QStringLiteral("fr/"));
    case QLocale::Italian:
        return QUrl(QString::fromLatin1(site) + QStringLiteral("it/"));
    default:
        return QUrl(QString::fromLatin1(site));
    }
}

QUrl AppInfo::helpUrl()
{
    return websiteUrl().resolved(QUrl(QStringLiteral("guide/")));
}
