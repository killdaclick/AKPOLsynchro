#-------------------------------------------------
#
# Project created by QtCreator 2015-07-02T09:18:30
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AKPOLsynchro
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
        AboutWindow.cpp \
        Utility.cpp

HEADERS  += MainWindow.h \
			AboutWindow.h \
			Utility.h

FORMS    += MainWindow.ui \
			AboutWindow.ui

RESOURCES     = resource.qrc

win32:RC_ICONS += icons/app/Treetog-Junior-Sync.ico
