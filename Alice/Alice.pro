#-------------------------------------------------
#
# Project created by QtCreator 2016-10-15T09:46:48
#
#-------------------------------------------------

QT       += core gui
QT += network
QT += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Alice
TEMPLATE = app

INCLUDEPATH += ../include

SOURCES += main.cpp\
        alice.cpp\

SOURCES += $$files(/*.cpp)

HEADERS  += alice.h

FORMS    += alice.ui

CONFIG += c++11
