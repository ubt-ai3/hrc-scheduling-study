#include <iostream>

#include <QApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QtGui>

#include <QtQml/qqmlextensionplugin.h>

#include "filtered_camera.h"

Q_IMPORT_QML_PLUGIN(ComponentsPlugin)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("University Bayreuth AI3");
    app.setOrganizationDomain("resy.uni-bayreuth.de");
    app.setApplicationName("Color Calibration");

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral(":/qt/qml"));

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/ColorCalibration/main.qml")));

    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
