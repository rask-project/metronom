#include "LanguageManager.h"

#include <QLocale>
#include <QSettings>

LanguageManager::LanguageManager(QObject *parent) : QObject(parent)
{
    const QString saved{QSettings().value(settingsKey).toString()};
    m_current = saved;
    m_applied = saved.isEmpty() ? systemCode() : saved;
}

QStringList LanguageManager::languages() const
{
    return QStringList(std::begin(names), std::end(names));
}

QStringList LanguageManager::languageCodes() const
{
    return QStringList(std::begin(codes), std::end(codes));
}

QString LanguageManager::currentLanguage() const
{
    return m_current.isEmpty() ? systemCode() : m_current;
}

bool LanguageManager::needsRestart() const
{
    return currentLanguage() != m_applied;
}

void LanguageManager::setLanguage(const QString &code)
{
    if (currentLanguage() == code) {
        return;
    }

    m_current = code;
    QSettings().setValue(settingsKey, code);
    emit currentLanguageChanged();
}

QString LanguageManager::systemCode()
{
    switch (QLocale::system().language()) {
    case QLocale::Portuguese:
        return QStringLiteral("pt_BR");
    case QLocale::Spanish:
        return QStringLiteral("es_ES");
    case QLocale::French:
        return QStringLiteral("fr_FR");
    case QLocale::Italian:
        return QStringLiteral("it_IT");
    default:
        return QStringLiteral("en");
    }
}
