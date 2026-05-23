#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include "TaskController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("CineIris");
    app.setApplicationVersion("2.0.0");
    app.setWindowIcon(QIcon(":/res/cineiris.svg"));

    TaskController controller;

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral(FLUENTUI_QML_DIR));
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.load(QUrl("qrc:/qml/Main.qml"));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
