#include "Appearance.h"

#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>
#include <utility>

Appearance::Appearance(QObject *parent) : QObject(parent), m_colorScheme(savedColorScheme())
{
    QGuiApplication::styleHints()->setColorScheme(m_colorScheme);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &Appearance::darkChanged);
}

Qt::ColorScheme Appearance::colorScheme() const noexcept
{
    return m_colorScheme;
}

void Appearance::setColorScheme(Qt::ColorScheme colorScheme)
{
    if (m_colorScheme == colorScheme) {
        return;
    }

    m_colorScheme = colorScheme;
    QSettings().setValue(settingsKey, std::to_underlying(colorScheme));
    QGuiApplication::styleHints()->setColorScheme(colorScheme);
    emit colorSchemeChanged();
    emit darkChanged();
}

bool Appearance::dark() const
{
    // Not every platform honours the override, so an explicit choice wins over what the system reports.
    if (m_colorScheme != Qt::ColorScheme::Unknown) {
        return m_colorScheme == Qt::ColorScheme::Dark;
    }
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

Qt::ColorScheme Appearance::savedColorScheme()
{
    QSettings settings;
    if (settings.contains(legacySettingsKey)) {
        const int legacy = settings.value(legacySettingsKey).toInt();
        settings.remove(legacySettingsKey);
        const Qt::ColorScheme migrated = legacy == 0   ? Qt::ColorScheme::Light
                                         : legacy == 1 ? Qt::ColorScheme::Dark
                                                       : Qt::ColorScheme::Unknown;
        settings.setValue(settingsKey, std::to_underlying(migrated));
        return migrated;
    }

    const int saved = settings.value(settingsKey).toInt();
    if (saved == std::to_underlying(Qt::ColorScheme::Light)) {
        return Qt::ColorScheme::Light;
    }
    if (saved == std::to_underlying(Qt::ColorScheme::Dark)) {
        return Qt::ColorScheme::Dark;
    }
    return Qt::ColorScheme::Unknown;
}
