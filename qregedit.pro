QT       += core gui widgets

TARGET = qregedit
TEMPLATE = app

CONFIG += warn_on link_pkgconfig c++17

# warn on *any* usage of deprecated APIs
DEFINES += QT_DEPRECATED_WARNINGS
# ... and just fail to compile if APIs deprecated in Qt <= 5.15 are used
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00

DEFINES += QT_NO_KEYWORDS QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_BYTEARRAY

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

isEmpty(INSTALL_PREFIX):INSTALL_PREFIX = /usr
TARGET       = qregedit
TARGET.files = qregedit
TARGET.path  = $$INSTALL_PREFIX/bin
INSTALLS    += TARGET desktop icons

desktop.files   = qregedit.desktop
desktop.path    = $$INSTALL_PREFIX/share/applications

icons.files = data/qregedit.png
icons.path  = $$INSTALL_PREFIX/share/icons
