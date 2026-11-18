#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

class AppInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString qtBuildVersion READ qtBuildVersion CONSTANT)
    // Only there a QML MenuBar becomes the system menu bar; elsewhere it would be drawn over the content.
    Q_PROPERTY(bool nativeMenuBar READ nativeMenuBar CONSTANT)
    // The website in the app's language, and its How to use page.
    Q_PROPERTY(QUrl websiteUrl READ websiteUrl CONSTANT)
    Q_PROPERTY(QUrl helpUrl READ helpUrl CONSTANT)

public:
    explicit AppInfo(QObject *parent = nullptr);

    static QString qtVersion();
    static QString qtBuildVersion();
    static bool nativeMenuBar() noexcept;
    static QUrl websiteUrl();
    static QUrl helpUrl();
};
