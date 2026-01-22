#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "jsonbridge.h"
#include "theme.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set application metadata
    QGuiApplication::setApplicationName("Airgap JSON Formatter");
    QGuiApplication::setOrganizationName("Airgap");
    QGuiApplication::setApplicationVersion("0.1.0");

    // Use Fusion style for consistent look across platforms
    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;

    // Create and expose JsonBridge to QML (Theme is now a QML singleton)
    JsonBridge jsonBridge;
    engine.rootContext()->setContextProperty("JsonBridge", &jsonBridge);

    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/AirgapFormatter/qml/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
