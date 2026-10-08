# softkeys' own tests. Built by `make test` only, never by `make`.

TEMPLATE = app
TARGET = softkeys-tests
QT += testlib widgets
CONFIG += console
CONFIG -= app_bundle

include(../softkeys.pri)

QMAKE_CXXFLAGS += -Wall -Wextra

SOURCES += main.cpp
