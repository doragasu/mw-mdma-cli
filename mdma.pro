######################################################################
# MDMA project with QT
######################################################################

TEMPLATE = app
TARGET = mdma
INCLUDEPATH += .

QT += widgets

DEFINES += QT_DEPRECATED_WARNINGS

# You can make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -lusb-1.0

DEFINES += QT

# Input
HEADERS = flashdlg.h commands.h esp-prog.h mdma.h progbar.h flash_man.h
SOURCES += main.cpp flashdlg.cpp commands.c esp-prog.c mdma.c progbar.c flash_man.cpp
