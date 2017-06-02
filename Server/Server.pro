#-------------------------------------------------
#
# Project created by QtCreator 2016-10-15T09:46:21
#
#-------------------------------------------------

QT       += core gui
QT += network
QT += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Server
TEMPLATE = app

INCLUDEPATH += ../include

SOURCES += $$files(../include/*.cpp)


SOURCES += main.cpp\
        server.cpp

HEADERS  += server.h

FORMS    += server.ui

CONFIG += c++11
