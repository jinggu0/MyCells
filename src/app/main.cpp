#include "AppController.h"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    AppController controller;

    const QString dbPath = QDir::current().absoluteFilePath("mycells.db");
    const QString initSqlPath = QDir::current().absoluteFilePath("scripts/init_db.sql");

    controller.initialize(dbPath, initSqlPath, "default-cell");

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&controller]() {
        controller.refresh();
    });

    engine.rootContext()->setContextProperty("appController", &controller);
    engine.loadFromModule("MyCells", "Main");

    if(engine.rootObjects().isEmpty())
    {
        return -1;
    }

    return app.exec();
}
