#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QtDebug>
#include <QFile>
#include <QTextStream>

#include "tensorflow.h"
#include "auxutils.h"
#include "objectsrecogfilter.h"

double AuxUtils::angleHor = 0;
double AuxUtils::angleVer = 0;
int    AuxUtils::width    = 0;
int    AuxUtils::height   = 0;

using namespace tflite;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("Mechatronics Blog");
    app.setOrganizationDomain("mechatronicsblog.com");
    app.setApplicationName("TFLite_Qt_Pi");

    QQmlApplicationEngine engine;

    // Register C++ QML types
    qmlRegisterType<TensorFlow>("TensorFlow", 1, 0, "TensorFlow");
    qmlRegisterType<ObjectsRecogFilter>("ObjectsRecognizer", 1, 0, "ObjectsRecognizer");
    qmlRegisterType<AuxUtils>("AuxUtils", 1, 0, "AuxUtils");

    // Global objects
    AuxUtils* auxUtils = new AuxUtils();
    engine.rootContext()->setContextProperty("auxUtils",auxUtils);
    engine.rootContext()->setContextProperty("globalEngine",&engine);

    // Register meta types
    qRegisterMetaType<QList<QRectF>>("QList<QRectF>");
    qRegisterMetaType<QList<QImage>>("QList<QImage>");

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
