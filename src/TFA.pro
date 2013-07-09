#-------------------------------------------------
#
# Project created by QtCreator 2013-02-14T20:04:19
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TFA
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    wavreader.cpp \
    srtwriter.cpp

HEADERS  += mainwindow.h \
    wavreader.h \
    srtwriter.h

FORMS    += mainwindow.ui

RESOURCES += TFA.qrc

RC_FILE = TFA.rc
