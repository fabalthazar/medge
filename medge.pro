#-------------------------------------------------
#
# Project created by QtCreator 2015-12-08T23:58:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = medge
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11


SOURCES += main.cpp\
        mainwindow.cpp \
    roi.cpp \
    graphics.cpp

HEADERS  += mainwindow.h \
    config.h \
    image.h \
    roi.h \
    graphics.h

FORMS    += mainwindow.ui

unix:!macx: LIBS += -L$$PWD/dcmtk/lib -ldcmdata -lz -loflog -lofstd

INCLUDEPATH += $$PWD/dcmtk/include
DEPENDPATH += $$PWD/dcmtk/include

unix:!macx: PRE_TARGETDEPS += $$PWD/dcmtk/lib/libdcmdata.a
unix:!macx: PRE_TARGETDEPS += $$PWD/dcmtk/lib/liboflog.a
unix:!macx: PRE_TARGETDEPS += $$PWD/dcmtk/lib/libofstd.a

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libxml-2.0
