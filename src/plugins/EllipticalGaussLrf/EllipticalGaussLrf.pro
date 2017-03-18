#-------------------------------------------------
#
# Project created by QtCreator 2017-03-01T12:47:03
#
#-------------------------------------------------

QT       += widgets

TARGET = EllipticalGaussLrf
TEMPLATE = lib

DEFINES += ELLIPTICALGAUSSLRF_LIBRARY
DEFINES += GUI

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


#---CERN ROOT---
win32 {
        INCLUDEPATH += c:/root/include
        LIBS += -Lc:/root/lib/ -llibCore -llibCint -llibRIO -llibNet -llibHist -llibGraf -llibGraf3d -llibGpad -llibTree -llibRint -llibPostscript -llibMatrix -llibPhysics -llibRint -llibMathCore -llibGeom -llibGeomPainter -llibGeomBuilder -llibMathMore -llibMinuit2 -llibThread
}
linux-g++ || unix {
        INCLUDEPATH += $$system(root-config --incdir)
}
#-----------

INCLUDEPATH += ../../modules/lrf_v3
INCLUDEPATH += ../../modules/lrf_v3/gui
INCLUDEPATH += ../../common #For aid.h

SOURCES += \
    ../../modules/lrf_v3/gui/atransformwidget.cpp \
    type.cpp \
    main.cpp \
    settingswidget.cpp \
    internalswidget.cpp \
    lrf.cpp

HEADERS += \
    ../../modules/lrf_v3/gui/atransformwidget.h \
    ellipticalgausslrf_global.h \
    type.h \
    settingswidget.h \
    internalswidget.h \
    lrf.h

FORMS += \
    settingswidget.ui \
    internalswidget.ui
