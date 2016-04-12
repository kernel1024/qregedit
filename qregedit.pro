QT       += core gui widgets

TARGET = qregedit
TEMPLATE = app

CONFIG += warn_on link_pkgconfig c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    chntpw/ntreg.c \
    registrymodel.cpp \
    global.cpp \
    regutils.cpp \
    valueeditor.cpp

HEADERS  += mainwindow.h \
    chntpw/ntreg.h \
    registrymodel.h \
    global.h \
    regutils.h \
    valueeditor.h

FORMS    += mainwindow.ui \
    valueeditor.ui

OTHER_FILES += \
    LICENSE \
    README.md

RESOURCES += \
    qregedit.qrc

include( qhexedit2/qhexedit.pri )
