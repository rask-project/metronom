#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationDomain(QStringLiteral("com.github.rask-project"));
    QCoreApplication::setOrganizationName(QStringLiteral("rask-project"));
    QCoreApplication::setApplicationName(QStringLiteral("Metronom"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QGuiApplication app(argc, argv);
    const QLocale systemLocale = QLocale::system();
    QLocale::setDefault(systemLocale);

    QGuiApplication::setWindowIcon(QIcon(":/resources/logo.png"));

    QTranslator translator;
    if (translator.load(
            systemLocale,
            QStringLiteral("metronom"),
            QString(),
            QStringLiteral("/i18n"),
            QStringLiteral(".qm")
        )) {
        app.installTranslator(&translator);
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
