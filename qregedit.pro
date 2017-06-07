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
    valueeditor.cpp \
    settingsdlg.cpp \
    functions.cpp \
    progressdialog.cpp \
    finder.cpp \
    logdisplay.cpp \
    chntpw/libsam.c \
    sammodel.cpp \
    userdialog.cpp

HEADERS  += mainwindow.h \
    chntpw/ntreg.h \
    registrymodel.h \
    global.h \
    regutils.h \
    valueeditor.h \
    settingsdlg.h \
    functions.h \
    progressdialog.h \
    finder.h \
    logdisplay.h \
    chntpw/sam.h \
    sammodel.h \
    userdialog.h

FORMS    += mainwindow.ui \
    valueeditor.ui \
    settingsdlg.ui \
    progressdialog.ui \
    logdisplay.ui \
    userdialog.ui \
    listdialog.ui

OTHER_FILES += \
    LICENSE \
    README.md

RESOURCES += \
    qregedit.qrc

include( qhexedit2/qhexedit.pri )

win32 {
    RC_FILE = qregedit.rc
}

