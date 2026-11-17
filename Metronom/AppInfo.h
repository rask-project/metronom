#pragma once

#include <QObject>
#include <QString>
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

public:
    explicit AppInfo(QObject *parent = nullptr);

    static QString qtVersion();
    static QString qtBuildVersion();
    static bool nativeMenuBar() noexcept;
};
