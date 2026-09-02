#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class StatusBarController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool lightStatusBar READ lightStatusBar WRITE setLightStatusBar NOTIFY lightStatusBarChanged)

public:
    explicit StatusBarController(QObject *parent = nullptr);

    bool lightStatusBar() const;
    void setLightStatusBar(bool light);

signals:
    void lightStatusBarChanged();

private:
    bool m_lightStatusBar = false;
};
