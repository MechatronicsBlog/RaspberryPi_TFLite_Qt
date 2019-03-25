QT += quick multimedia sensors multimedia-private
CONFIG += console c++11 qml_debug

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

INCLUDEPATH += $$PWD/cpp \
               $$PWD/../tensorflow \
               $$PWD/../tensorflow/tensorflow/lite/tools/make/downloads/flatbuffers/include

# We consider Linux and distinguish between Raspbian (for Raspberry Pi) and other Linux distributions
linux{
    contains(QMAKE_CXX, .*raspbian.*arm.*):{
        # TensorFlow Lite lib path
        LIBS += -L$$PWD/../tensorflow/tensorflow/lite/tools/make/gen/rpi_armv7l/lib

        # Assets to be deployed: path and files
        assets.path = /home/pi/qt_apps/$${TARGET}/bin/assets
        assets.files = assets/*
    }
    else {
        # TensorFlow Lite lib path
        LIBS += -L$$PWD/../tensorflow/tensorflow/lite/tools/make/gen/linux_x86_64/lib

        # Assets to be deployed: path and files
        # WARNING: Define yourself the path!
        # assets.path = /home/user/app
        assets.files = assets/*
    }
}

LIBS += -ltensorflow-lite -ldl

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/pi/qt_apps/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
INSTALLS += assets

HEADERS += \
    cpp/objectsrecogfilter.h \
    cpp/tensorflow.h \
    cpp/tensorflowthread.h \
    cpp/colormanager.h \
    cpp/get_top_n.h \
    cpp/auxutils.h

SOURCES += \
    main.cpp \
    cpp/objectsrecogfilter.cpp \
    cpp/tensorflow.cpp \
    cpp/tensorflowthread.cpp \
    cpp/colormanager.cpp \
    cpp/auxutils.cpp

DISTFILES += \
    Home.qml \
    Configuration.qml \
    main.qml
