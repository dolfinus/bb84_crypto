#-------------------------------------------------
#
# Project created by QtCreator 2016-10-15T09:46:48
#
#-------------------------------------------------

QT       += core gui
QT += network
QT += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Eve
TEMPLATE = app

SOURCES += $$files(../include/*.cpp)

INCLUDEPATH += ../include

SOURCES += main.cpp\
        eve.cpp

HEADERS  += eve.h

FORMS    += eve.ui

CONFIG += c++11
