QT = core gui qml testlib
CONFIG += console testcase c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_nativedeckcontroller
CONFIG += link_pkgconfig
PKGCONFIG += sdl2
INCLUDEPATH += ../../app
SOURCES += tst_nativedeckcontroller.cpp \
           ../../app/settings/streamingpreferences.cpp \
           ../../app/streaming/vrrratepolicy.cpp
HEADERS += ../../app/settings/nativedeckcontroller.h \
           ../../app/settings/streamingpreferences.h
