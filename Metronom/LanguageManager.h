#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>
#include <array>

class LanguageManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList languages READ languages CONSTANT)
    Q_PROPERTY(QStringList languageCodes READ languageCodes CONSTANT)
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY currentLanguageChanged)
    Q_PROPERTY(bool needsRestart READ needsRestart NOTIFY currentLanguageChanged)

public:
    explicit LanguageManager(QObject *parent = nullptr);

    QStringList languages() const;
    QStringList languageCodes() const;
    QString currentLanguage() const;
    bool needsRestart() const;

    Q_INVOKABLE void setLanguage(const QString &code);

signals:
    void currentLanguageChanged();

private:
    static QString systemCode();

    static constexpr std::array<const char *, 5> codes{"en", "pt_BR", "es_ES", "fr_FR", "it_IT"};
    static constexpr std::array<const char *, 5>
        names{"English", "Português (Brasil)", "Español", "Français", "Italiano"};
    static constexpr const char *settingsKey = "language";

    QString m_applied;
    QString m_current;
};
