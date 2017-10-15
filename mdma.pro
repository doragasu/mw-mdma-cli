######################################################################
# MDMA qmake project file.
#
# doragasu, 2017
######################################################################

TEMPLATE = app
TARGET = mdma
INCLUDEPATH += .

QT += widgets

DEFINES += QT_DEPRECATED_WARNINGS

# Uncomment following three lines for Windows static builds
#CONFIG+=static
#CONFIG+=no_smart_library_merge
#QTPLUGIN+=qwindows

# Link with libusb-1.0
LIBS += -lusb-1.0

DEFINES += QT

# Input files
HEADERS = flashdlg.h commands.h esp-prog.h mdma.h progbar.h flash_man.h
SOURCES += main.cpp flashdlg.cpp commands.c esp-prog.c mdma.c progbar.c flash_man.cpp
