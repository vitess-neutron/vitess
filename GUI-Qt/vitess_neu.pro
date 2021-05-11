#-------------------------------------------------
#
# Project created by QtCreator 2020-10-28T10:39:26
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Vitess-Qt
TEMPLATE = app

OBJECTS_DIR = .obj
MOC_DIR = .moc

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += ../SRC
INCLUDEPATH += C:\Programmieren\yaml-cpp\include

CONFIG += c++11

SOURCES += \
        big.cpp \
        chopperphases.cpp \
        chrystanalyzer.cpp \
        help.cpp \
        main.cpp \
        mainwindow.cpp \
        modultable.cpp \
        parameter.cpp \
        progress.cpp \
        tools.cpp \
        ../SRC/convert.c

HEADERS += \
        big.h \
        chopperphases.h \
        chrystanalyzer.h \
        help.h \
        mainwindow.h \
        modultable.h \
        parameter.h \
        progress.h \
        tools.h
FORMS += \
        big.ui \
        chopperphases.ui \
        chrystanalyzer.ui \
        help.ui \
        mainwindow.ui \
        modultable.ui \
        parameter.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: LIBS += -L"C:\Programmieren\yaml-cpp\src\build" -lyaml-cpp

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += yaml-cpp

RESOURCES += \
    resource.qrc

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../yaml-cpp/src/build/ -lyaml-cpp
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../yaml-cpp/src/build/ -lyaml-cppd
#else:unix: LIBS += -L$$PWD/../../yaml-cpp/src/build/ -lyaml-cpp

INCLUDEPATH += $$PWD/../../yaml-cpp/src/build
DEPENDPATH += $$PWD/../../yaml-cpp/src/build
