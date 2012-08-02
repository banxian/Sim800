#-------------------------------------------------
#
# Project created by QtCreator 2010-05-08T14:53:58
#
#-------------------------------------------------

QT       += core gui webkit xml sql

#TARGET = Sim800
TEMPLATE = app

CONFIG += qt \
    debug_and_relase
CONFIG(debug, debug|release) {
    TARGET = Sim800_d
    DESTDIR = ../Target/GCC/Debug
    MOC_DIR += ./tmp/moc/Debug
    OBJECTS_DIR += ./tmp/obj/Debug
}
else {
    TARGET = Sim800
    DESTDIR = ../Target/GCC/Release
    MOC_DIR += ./tmp/moc/Release
    OBJECTS_DIR += ./tmp/obj/Release
}
DEPENDPATH += .
UI_DIR += ./tmp
RCC_DIR += ./tmp
INCLUDEPATH += ./tmp \
    $MOC_DIR \
    . \
    ./../FvQKOLLite
LIBS += -lole32 \
    -L"./PpsDES"


SOURCES += main.cpp\
        MainUnt.cpp \
    DbCentre.cpp \
    MyDropTreeWidget.cpp \
    NewBookUnt.cpp \
    SettingsUnt.cpp \
    ReaderUnt.cpp \
    MyZoomWidget.cpp \
    KeypadUnt.cpp

HEADERS  += MainUnt.h \
    DbCentre.h \
    MyDropTreeWidget.h \
    NewBookUnt.h \
    SettingsUnt.h \
    ReaderUnt.h \
    MyZoomWidget.h \
    KeypadUnt.h

FORMS    += MainFrm.ui \
    KeypadFrm.ui

RESOURCES += \
    Sim800.qrc

OTHER_FILES += \
    Sim800_chs.ts
