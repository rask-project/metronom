#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationDomain(QStringLiteral("com.github.rask-project"));
    QCoreApplication::setOrganizationName(QStringLiteral("rask-project"));
    QCoreApplication::setApplicationName(QStringLiteral("Metronom"));
    QCoreApplication::setApplicationVersion(
        QStringLiteral(METRONOM_VERSION) + QStringLiteral(" (") + QStringLiteral(METRONOM_GIT_COMMIT) +
        QStringLiteral(")")
    );

    QGuiApplication app(argc, argv);
    QGuiApplication::setWindowIcon(QIcon(":/resources/logo.png"));

    const QString chosenLanguage = QSettings().value("language").toString();
    const QLocale locale = chosenLanguage.isEmpty() ? QLocale::system() : QLocale(chosenLanguage);
    QLocale::setDefault(locale);

    QTranslator translator;
    const QString qmPath = QStringLiteral(":/i18n/metronom_") + locale.name() + QStringLiteral(".qm");
    if (translator.load(qmPath)) {
        app.installTranslator(&translator);
        qInfo().noquote() << "Translation loaded:" << locale.name();
    } else {
        qWarning().noquote() << "No translation for locale:" << locale.name();
    }

    QQmlApplicationEngine engine;

    QIcon::setThemeSearchPaths({":/icons"});
    QIcon::setThemeName(QStringLiteral("metronom"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );
    engine.loadFromModule("App", "Splash");

    return QGuiApplication::exec();
}
