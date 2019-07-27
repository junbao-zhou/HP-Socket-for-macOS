#-------------------------------------------------
#
# Project created by QtCreator 2019-07-26T22:58:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EchoServer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

CONFIG += c++14

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    tcpserverlistenerimpl.cpp

HEADERS += \
        mainwindow.h \
    tcpserverlistenerimpl.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../../includes/
DEPENDPATH += $$PWD/../../includes/

macx: LIBS += -L$$PWD/../../lib/ -lhpsocket
