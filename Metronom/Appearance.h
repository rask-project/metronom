#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

// Light/dark choice of the user (Qt::ColorScheme::Unknown follows the system), applied to the whole application.
class Appearance : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Qt::ColorScheme colorScheme READ colorScheme WRITE setColorScheme NOTIFY colorSchemeChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY darkChanged)

public:
    explicit Appearance(QObject *parent = nullptr);

    Qt::ColorScheme colorScheme() const noexcept;
    void setColorScheme(Qt::ColorScheme colorScheme);

    bool dark() const;

signals:
    void colorSchemeChanged();
    void darkChanged();

private:
    static Qt::ColorScheme savedColorScheme();

    static constexpr const char *settingsKey = "colorScheme";
    // Material theme of versions up to 2.0.0: 0 light, 1 dark, 2 system.
    static constexpr const char *legacySettingsKey = "theme";

    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
};
