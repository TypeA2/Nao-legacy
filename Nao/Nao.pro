#-------------------------------------------------
#
# Project created by QtCreator 2018-03-10T12:10:47
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Nao
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

win32:RC_ICONS += Nao.ico

SOURCES += \
        main.cpp \
        NMain.cpp

HEADERS += \
        NMain.h

INCLUDEPATH += $$PWD/../../libnao/libnao

CONFIG(debug, debug|release) {
    LIBS += -L"$$PWD/../../libnao/libnao-debug/debug" -llibnao
}

CONFIG(release, debug|release) {
    LIBS += -L"$$PWD/../../libnao/libnao-release/release" -llibnao
}
