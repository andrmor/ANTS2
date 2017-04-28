#--------------ANTS2--------------
ANTS2_MAJOR = 3
ANTS2_MINOR = 12
ANTS2_VERSION = 2180

#Optional libraries
#CONFIG += ants2_cuda        #enable CUDA support - need NVIDIA GPU and drivers (CUDA toolkit) installed!
#CONFIG += ants2_flann       #enable FLANN (fast neighbour search) library
#CONFIG += ants2_fann        #enables FANN (fast neural network) library
CONFIG += ants2_eigen3      #use Eigen3 library instead of ROOT for linear algebra

#CONFIG += ants2_RootServer  #enable cern CERN ROOT html server --- EXPERIMENTAL FEATURE

#---CERN ROOT---
win32 {
     INCLUDEPATH += c:/root/include
     LIBS += -Lc:/root/lib/ -llibCore -llibCint -llibRIO -llibNet -llibHist -llibGraf -llibGraf3d -llibGpad -llibTree -llibRint -llibPostscript -llibMatrix -llibPhysics -llibRint -llibMathCore -llibGeom -llibGeomPainter -llibGeomBuilder -llibMathMore -llibMinuit2 -llibThread
     ants2_RootServer {LIBS += -llibRHTTP}
}
linux-g++ || unix {
     INCLUDEPATH += $$system(root-config --incdir)
     LIBS += $$system(root-config --libs) -lGeom -lGeomPainter -lGeomBuilder -lMathMore -lMinuit2
     ants2_RootServer {LIBS += -llibRHTTP}
}
#-----------

#---EIGEN---
ants2_eigen3 {
     DEFINES += USE_EIGEN

     win32 { INCLUDEPATH += C:/eigen3 }
     linux-g++ || unix { INCLUDEPATH += /usr/include/eigen3 }

     #advanced options:
     DEFINES += NEWFIT #if enabled, use advanced fitting class (non-negative LS, non-decreasing LS, hole-plugging, etc.)
     #DEFINES += TPS3M  #if enabled - use matrix algebra for TP splines - UNDER DEVELOPMENT

     SOURCES += SplineLibrary/bs3fit.cpp \
                SplineLibrary/tps3fit.cpp \
                SplineLibrary/tpspline3m.cpp

     HEADERS += SplineLibrary/bs3fit.h \
                SplineLibrary/tps3fit.h \
                SplineLibrary/tpspline3m.h
}
#----------

#---FLANN---
ants2_flann {
     DEFINES += ANTS_FLANN

     win32 {
        LIBS += -LC:/FLANN/lib -lflann
        INCLUDEPATH += C:/FLANN/include
     }
}
#----------

#---FANN---
ants2_fann {
     DEFINES += ANTS_FANN

     win32 {
        LIBS += -Lc:/FANN-2.2.0-Source/bin/ -lfannfloat
        INCLUDEPATH += c:/FANN-2.2.0-Source/src/include
        DEFINES += NOMINMAX
     }
     linux-g++ || unix { LIBS += -lfann }
}
#---------

#---CUDA---
ants2_cuda {
    DEFINES += __USE_ANTS_CUDA__

    #Path to CUDA toolkit:
    win32 { CUDA_DIR = "C:/cuda/toolkit" }        # AVOID SPACES IN THE PATH during toolkit installation!
    linux-g++ || unix { CUDA_DIR = $$system(which nvcc | sed 's,/bin/nvcc$,,') }

    CUDA_SOURCES += CUDA/cudaANTS.cu
    OTHER_FILES += CUDA/cudaANTS.cu
    CUDA_OBJECTS_DIR = release/cuda
    INCLUDEPATH += CUDA
    NVCC_OPTIONS = --use_fast_math

    LIBS += -lcuda -lcudart #-lcusolver -lcublas -lcublas_device -lcudadevrt -lcufft -lcufftw -lcurand -lcusparse -lnppc -lnppi -lnpps -lnvcuvenc -lnvcuvid

    unix {
        NVCC_BIN = nvcc
        LIBS += -L$$CUDA_DIR/lib64                                # Library path (remove 64 on 32bit)
        debug:NVCC_OPTIONS += -D_DEBUG
        NVCC_COMPILER_FLAGS = -Xcompiler $$QMAKE_CXXFLAGS -std=c++11 -D__CORRECT_ISO_CPP11_MATH_H_PROTO

        # Configuration of the Cuda compiler
        cuda.input = CUDA_SOURCES
        cuda.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
        cuda.commands = $$NVCC_BIN $$NVCC_OPTIONS $$NVCC_COMPILER_FLAGS -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
        cuda.dependency_type = TYPE_C
        QMAKE_EXTRA_COMPILERS += cuda
    }
    win32 {
        #NVCC_BIN = $$CUDA_DIR/bin/nvcc.exe  # Location of nvcc
        #CUDA_ARCH = sm_13                   # Type of CUDA architecture, for example 'compute_10', 'compute_11', 'sm_10'
        SYSTEM_NAME = Win32                 # Depending on your system either 'Win32' or 'x64'
        SYSTEM_TYPE = 32                    # '32' or '64'; can be '32' on 64bit system
        #debug:NVCC_OPTIONS += -D_DEBUG
        #NVCC_COMPILER_FLAGS += -Xcompiler -MD --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH
        LIBS += -L$$CUDA_DIR/lib/Win32
        #INCLUDEPATH += $$CUDA_DIR/include

        CONFIG(debug, debug|release){
         # Debug mode
         cuda_d.input = CUDA_SOURCES
         cuda_d.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
         cuda_d.commands = $$CUDA_DIR/bin/nvcc.exe -D_DEBUG -Xcompiler -MD $$NVCC_OPTIONS $$CUDA_INC $$LIBS --machine $$SYSTEM_TYPE -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
         cuda_d.dependency_type = TYPE_C
         QMAKE_EXTRA_COMPILERS += cuda_d
        }
        else{
         # Release mode
         cuda.input = CUDA_SOURCES
         cuda.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o  #cuda.output = release/cuda/${QMAKE_FILE_BASE}_cuda.o
         #cuda.commands = $$CUDA_DIR/bin/nvcc.exe -Xcompiler -MD $$NVCC_OPTIONS $$CUDA_INC $$LIBS --machine $$SYSTEM_TYPE -arch=compute_35 -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
         cuda.commands = $$CUDA_DIR/bin/nvcc.exe -Xcompiler -MD $$NVCC_OPTIONS $$CUDA_INC $$LIBS --machine $$SYSTEM_TYPE -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
                                                                                                                         #-arch=sm20
         cuda.dependency_type = TYPE_C
         QMAKE_EXTRA_COMPILERS += cuda
        }
     }
}

ants2_RootServer{
  DEFINES += USE_ROOT_HTML
}

#----------

#Modular compilation flags
CONFIG += ants2_GUI         #if disabled, GUI is not compiled
CONFIG += ants2_SIM         #if disabled, simulation-related modules are not compiled

#---ANTS2 files---
SOURCES += main.cpp \
    common/jsonparser.cpp \
    common/CorrelationFilters.cpp \
    common/reconstructionsettings.cpp \
    common/generalsimsettings.cpp \
    common/pmtypeclass.cpp \
    common/globalsettingsclass.cpp \
    common/tmpobjhubclass.cpp \
    common/ajsontools.cpp \
    common/afiletools.cpp \
    common/amessage.cpp \
    common/acommonfunctions.cpp \
    common/aconfiguration.cpp \
    common/apreprocessingsettings.cpp \
    common/aeventfilteringsettings.cpp \
    common/apmposangtyperecord.cpp \
    common/apmarraydata.cpp \
    common/aqtmessageredirector.cpp \
    common/ageoobject.cpp \
    common/aopticaloverride.cpp \
    modules/detectorclass.cpp \
    modules/pms.cpp \
    modules/eventsdataclass.cpp \
    modules/dynamicpassiveshandler.cpp \
    modules/reconstructionmanagerclass.cpp \
    modules/processorclass.cpp \
    modules/neuralnetworksmodule.cpp \
    modules/nnmoduleclass.cpp \
    modules/particlesourcesclass.cpp \
    modules/flatfield.cpp \
    modules/sensorlrfs.cpp \
    modules/manifesthandling.cpp \
    modules/apmgroupsmanager.cpp \
    modules/asandwich.cpp \
    SplineLibrary/spline.cpp \
    SplineLibrary/bspline.cpp \
    SplineLibrary/bspline3.cpp \
    modules/lrf_v2/pmsensor.cpp \
    modules/lrf_v2/lrf2.cpp \
    modules/lrf_v2/lrfcaxial.cpp \
    modules/lrf_v2/lrfaxial.cpp \
    modules/lrf_v2/pmsensorgroup.cpp \
    modules/lrf_v2/lrfxy.cpp \
    modules/lrf_v2/lrfcomposite.cpp \
    modules/lrf_v2/lrfsliced3d.cpp \
    modules/lrf_v2/lrfaxial3d.cpp \
    modules/lrf_v2/lrfcaxial3d.cpp \
    modules/lrf_v2/lrf3d.cpp \
    modules/lrf_v2/sensorlocalcache.cpp \
    modules/lrf_v2/lrffactory.cpp \
    modules/lrf_v2/alrffitsettings.cpp \
    CUDA/cudamanagerclass.cpp \
    scriptmode/interfacetoglobscript.cpp \
    scriptmode/scriptminimizer.cpp \
    scriptmode/scriptinterfaces.cpp \
    scriptmode/ascriptexample.cpp \
    scriptmode/ascriptexampledatabase.cpp \
    scriptmode/ascriptmanager.cpp \
    gui/MainWindowTools/slab.cpp \
    modules/lrf_v3/arecipe.cpp \
    modules/lrf_v3/alrftypemanager.cpp \
    modules/lrf_v3/arepository.cpp \
    modules/lrf_v3/corelrfs.cpp \
    modules/lrf_v3/corelrfstypes.cpp \
    modules/lrf_v3/asensor.cpp \
    modules/lrf_v3/ainstruction.cpp \
    modules/lrf_v3/ainstructioninput.cpp \
    modules/lrf_v3/afitlayersensorgroup.cpp \
    modules/alrfmoduleselector.cpp \
    common/acollapsiblegroupbox.cpp \
    common/ascriptvaluecopier.cpp \
    gui/MainWindowTools/arootmarkerconfigurator.cpp \
    common/acustomrandomsampling.cpp \
    gui/ahighlighters.cpp \
    common/amaterial.cpp \
    common/aparticle.cpp \
    modules/amaterialparticlecolection.cpp\
    common/ascriptvalueconverter.cpp \
    scriptmode/ainterfacetowebsocket.cpp \
    common/ainternetbrowser.cpp \
    Net/aroothttpserver.cpp \
    Net/anetworkmodule.cpp \
    Net/awebsocketserver.cpp \
    modules/lrf_v3/gui/atpspline3widget.cpp \
    modules/lrf_v3/gui/avladimircompressionwidget.cpp \
    SplineLibrary/tpspline3.cpp

HEADERS  += common/CorrelationFilters.h \
    common/jsonparser.h \
    common/reconstructionsettings.h \
    common/generalsimsettings.h \
    common/pmtypeclass.h \
    common/globalsettingsclass.h \
    common/tmpobjhubclass.h \
    common/agammarandomgenerator.h \
    common/apositionenergyrecords.h \
    common/ajsontools.h \
    common/afiletools.h \
    common/amessage.h \
    common/acommonfunctions.h \
    common/aconfiguration.h \
    common/apreprocessingsettings.h \
    common/aqtmessageredirector.h \
    common/aeventfilteringsettings.h \
    common/apmposangtyperecord.h \
    common/apmarraydata.h \
    common/ageoobject.h \
    common/aopticaloverride.h \
    modules/detectorclass.h \
    modules/pms.h \
    modules/particlesourcesclass.h \
    modules/nnmoduleclass.h \
    modules/flatfield.h \
    modules/neuralnetworksmodule.h \
    modules/sensorlrfs.h \
    modules/eventsdataclass.h \
    modules/dynamicpassiveshandler.h \
    modules/reconstructionmanagerclass.h \
    modules/processorclass.h \
    modules/manifesthandling.h \
    modules/apmgroupsmanager.h \
    SplineLibrary/bspline3nu.h \
    SplineLibrary/spline.h \
    SplineLibrary/bspline.h \
    SplineLibrary/bspline3.h \
    modules/lrf_v2/pmsensor.h \
    modules/lrf_v2/lrf2.h \
    modules/lrf_v2/lrfaxial.h \
    modules/lrf_v2/lrfcaxial.h \
    modules/lrf_v2/pmsensorgroup.h \
    modules/lrf_v2/lrfxy.h \
    modules/lrf_v2/lrfcomposite.h \
    modules/lrf_v2/lrfsliced3d.h \
    modules/lrf_v2/lrfaxial3d.h \
    modules/lrf_v2/lrfcaxial3d.h \
    modules/lrf_v2/lrf3d.h \
    modules/lrf_v2/sensorlocalcache.h \
    modules/lrf_v2/lrffactory.h \
    modules/lrf_v2/alrffitsettings.h \
    CUDA/cudamanagerclass.h \
    scriptmode/interfacetoglobscript.h \
    scriptmode/scriptminimizer.h \
    scriptmode/scriptinterfaces.h \
    scriptmode/ascriptexample.h \
    scriptmode/ascriptexampledatabase.h \
    scriptmode/ascriptmanager.h \
    scriptmode/ascriptinterface.h \
    modules/asandwich.h \
    gui/MainWindowTools/slab.h \
    common/aid.h \
    common/apoint.h \
    common/atransform.h \
    modules/lrf_v3/arecipe.h \
    modules/lrf_v3/alrf.h \
    modules/lrf_v3/alrftype.h \
    modules/lrf_v3/alrftypemanager.h \
    modules/lrf_v3/ainstruction.h \
    modules/lrf_v3/ainstructioninput.h \
    modules/lrf_v3/arepository.h \
    modules/lrf_v3/asensor.h \
    modules/lrf_v3/aversionhistory.h \
    modules/lrf_v3/avladimircompression.h \
    modules/lrf_v3/corelrfs.h \
    modules/lrf_v3/corelrfstypes.h \
    modules/lrf_v3/afitlayersensorgroup.h \
    modules/lrf_v3/idclasses.h \
    modules/lrf_v3/alrftypemanagerinterface.h \
    modules/lrf_v3/astateinterface.h \
    modules/alrfmoduleselector.h \
    common/acollapsiblegroupbox.h \
    common/ascriptvaluecopier.h \
    gui/MainWindowTools/arootmarkerconfigurator.h \
    common/acustomrandomsampling.h \
    gui/ahighlighters.h \
    common/amaterial.h \
    common/aparticle.h \
    modules/amaterialparticlecolection.h\
    common/ascriptvalueconverter.h \
    scriptmode/ainterfacetowebsocket.h \
    common/ainternetbrowser.h \
    Net/aroothttpserver.h \
    Net/anetworkmodule.h \
    Net/awebsocketserver.h \
    modules/lrf_v3/gui/atpspline3widget.h \
    modules/lrf_v3/gui/avladimircompressionwidget.h \
    SplineLibrary/eiquadprog.hpp \
    SplineLibrary/tpspline3.h

# --- SIM ---
ants2_SIM {
    DEFINES += SIM

    SOURCES += common/asimulationstatistics.cpp \
    modules/primaryparticletracker.cpp \
    modules/s1_generator.cpp \
    modules/photon_generator.cpp \
    modules/s2_generator.cpp \
    modules/oneeventclass.cpp \
    modules/simulationmanager.cpp \
    modules/phscatclaudiomodel.cpp \
    modules/scatteronmetal.cpp \
    modules/aphotontracer.cpp \
    modules/acompton.cpp \
    modules/ageometrytester.cpp

    HEADERS  += common/agridelementrecord.h \
    common/scanfloodstructure.h \
    common/aphoton.h \
    common/ageomarkerclass.h \
    common/atrackrecords.h \
    common/dotstgeostruct.h \
    common/aenergydepositioncell.h \
    common/aparticleonstack.h \
    common/ahistoryrecords.h \
    common/asimulationstatistics.h \
    modules/primaryparticletracker.h \
    modules/s1_generator.h \
    modules/photon_generator.h \
    modules/s2_generator.h \
    modules/oneeventclass.h \
    modules/simulationmanager.h \
    modules/phscatclaudiomodel.h \
    modules/scatteronmetal.h \
    modules/aphotontracer.h \
    modules/acompton.h \
    modules/ageometrytester.h
}

# --- GUI ---
ants2_GUI {
    DEFINES += GUI

    SOURCES += gui/mainwindow.cpp \
    gui/materialinspectorwindow.cpp \
    gui/outputwindow.cpp \
    gui/guiutils.cpp \
    gui/lrfwindow.cpp \
    gui/reconstructionwindow.cpp \
    gui/windownavigatorclass.cpp \
    gui/MainWindowTools/MainWindowDiskIO.cpp \
    gui/MainWindowTools/MainWindowPhotonSource.cpp \
    gui/neuralnetworkswindow.cpp \
    gui/MainWindowTools/MainWindowParticleSimulation.cpp \
    gui/MainWindowTools/MainWindowDetectorConstructor.cpp \
    gui/MainWindowTools/MainWindowMenu.cpp \
    gui/MainWindowTools/MainWindowTests.cpp \
    gui/MainWindowTools/MainWindowInits.cpp \
    gui/ReconstructionWindowTools/Reconstruction_Tracks.cpp \
    gui/exampleswindow.cpp \
    gui/detectoraddonswindow.cpp \
    gui/checkupwindowclass.cpp \
    gui/gainevaluatorwindowclass.cpp \
    gui/RasterWindow/rasterwindowbaseclass.cpp \
    gui/graphwindowclass.cpp \
    gui/geometrywindowclass.cpp \
    gui/RasterWindow/rasterwindowgraphclass.cpp \
    gui/ReconstructionWindowTools/reconstruction_diskio.cpp \
    gui/ReconstructionWindowTools/Reconstruction_Init.cpp \
    gui/ReconstructionWindowTools/Reconstruction_ResolutionAnalysis.cpp\
    gui/MainWindowTools/MainWindowParticles.cpp \
    gui/ReconstructionWindowTools/Reconstruction_NN.cpp \
    gui/viewer2darrayobject.cpp \
    gui/shapeablerectitem.cpp \
    gui/genericscriptwindowclass.cpp \
    gui/graphicsruler.cpp \
    gui/credits.cpp \
    gui/globalsettingswindowclass.cpp \
    scriptmode/interfacetocheckerscript.cpp \
    gui/MainWindowTools/mainwindowscatterfit.cpp \
    gui/MainWindowTools/globalscript.cpp \
    gui/MainWindowTools/aslablistwidget.cpp \
    gui/MainWindowTools/arootlineconfigurator.cpp \
    gui/MainWindowTools/ageotreewidget.cpp \
    gui/MainWindowTools/ashapehelpdialog.cpp \
    gui/MainWindowTools/agridelementdialog.cpp \
    gui/ascriptexampleexplorer.cpp \
    gui/MainWindowTools/mainwindowjson.cpp \
    gui/ReconstructionWindowTools/reconstructionwindowguiupdates.cpp \
    common/completingtexteditclass.cpp \
    gui/ascriptwindow.cpp \
    gui/alrfmouseexplorer.cpp \
    modules/lrf_v3/gui/atransformwidget.cpp \
    modules/lrf_v3/gui/abspline3widget.cpp \
    modules/lrf_v3/gui/corelrfswidgets.cpp \
    modules/lrf_v3/gui/alrfwindowwidgets.cpp \
    modules/lrf_v3/gui/alrfwindow.cpp \
    gui/alrfdraw.cpp

HEADERS  += gui/mainwindow.h \
    gui/materialinspectorwindow.h \
    gui/outputwindow.h \
    gui/guiutils.h \
    gui/lrfwindow.h \
    gui/reconstructionwindow.h \
    gui/windownavigatorclass.h \
    gui/neuralnetworkswindow.h \
    gui/exampleswindow.h \
    gui/detectoraddonswindow.h \
    gui/checkupwindowclass.h \
    gui/gainevaluatorwindowclass.h \
    gui/RasterWindow/rasterwindowbaseclass.h \
    gui/graphwindowclass.h \
    gui/geometrywindowclass.h \
    gui/RasterWindow/rasterwindowgraphclass.h \
    gui/viewer2darrayobject.h \
    gui/shapeablerectitem.h \
    gui/genericscriptwindowclass.h \
    gui/graphicsruler.h \
    gui/credits.h \
    gui/globalsettingswindowclass.h \
    scriptmode/interfacetocheckerscript.h \
    gui/MainWindowTools/aslablistwidget.h \
    gui/MainWindowTools/arootlineconfigurator.h \
    gui/MainWindowTools/ageotreewidget.h \
    gui/MainWindowTools/ashapehelpdialog.h \
    gui/MainWindowTools/agridelementdialog.h \
    gui/ascriptexampleexplorer.h \
    common/completingtexteditclass.h \
    gui/ascriptwindow.h \
    gui/alrfmouseexplorer.h \
    modules/lrf_v3/gui/atransformwidget.h \
    modules/lrf_v3/gui/abspline3widget.h \
    modules/lrf_v3/gui/corelrfswidgets.h \
    modules/lrf_v3/gui/alrfwindowwidgets.h \
    modules/lrf_v3/gui/alrfwindow.h \
    gui/alrfdraw.h


FORMS += gui/mainwindow.ui \
    gui/materialinspectorwindow.ui \
    gui/outputwindow.ui \
    gui/lrfwindow.ui \
    gui/reconstructionwindow.ui \
    gui/windownavigatorclass.ui \
    gui/neuralnetworkswindow.ui \
    gui/exampleswindow.ui \
    gui/detectoraddonswindow.ui \
    gui/checkupwindowclass.ui \
    gui/gainevaluatorwindowclass.ui \
    gui/graphwindowclass.ui \
    gui/geometrywindowclass.ui \
    gui/genericscriptwindowclass.ui \
    gui/credits.ui \
    gui/globalsettingswindowclass.ui \
    gui/MainWindowTools/ashapehelpdialog.ui \
    gui/MainWindowTools/agridelementdialog.ui \
    gui/ascriptexampleexplorer.ui \
    gui/ascriptwindow.ui \
    modules/lrf_v3/gui/alrfwindow.ui

INCLUDEPATH += gui
INCLUDEPATH += gui/RasterWindow
INCLUDEPATH += gui/ReconstructionWindowTools
INCLUDEPATH += modules/lrf_v3/gui
}

INCLUDEPATH += gui/MainWindowTools
INCLUDEPATH += SplineLibrary
INCLUDEPATH += modules
INCLUDEPATH += modules/lrf_v2
INCLUDEPATH += modules/lrf_v3
INCLUDEPATH += common
INCLUDEPATH += scriptmode
INCLUDEPATH += Net

RESOURCES += \
    Resources.qrc
#-------------

#---Qt kitchen---
QT += core gui
QT += websockets
QT += widgets
QT += script #scripts support
win32:QT += winextras  #used in windownavigator only

CONFIG += c++11

TARGET = ants2
TEMPLATE = app

RC_FILE = myapp.rc
#------------

#---Windows-specific compilation mode and warning suppression
win32 {
  #if the next 2 instructions are not commented, there is no optimization during compilation: drastic shortening of compilation, but ~20% performance loss
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE *= -Od

  #CONFIG   += console                  #enable to add standalone console for Windows
  DEFINES  += _CRT_SECURE_NO_WARNINGS   #disable microsoft spam
  #DEFINES += WINDOWSBIN                #enable for compilation in Windows binary-only mode
}
#------------

#---Additional defines---
DEFINES += ANTS2_MINOR=\"$$ANTS2_MINOR\"
DEFINES += ANTS2_MAJOR=\"$$ANTS2_MAJOR\"
DEFINES += ANTS2_VERSION=\"$$ANTS2_VERSION\"
win32 {
  DEFINES += BUILDTIME=\\\"$$system('echo %time%')\\\"
  DEFINES += BUILDDATE=\\\"$$system('echo %date%')\\\"
} else {
  DEFINES += BUILDTIME=\\\"$$system(date '+%H:%M.%s')\\\"
  DEFINES += BUILDDATE=\\\"$$system(date '+%d/%m/%y')\\\"
}
#------------

#---Copy EXAMPLES directory from source to working directory---
win32 {
        #todir = $${OUT_PWD}/../EXAMPLES
        todir = $${OUT_PWD}/EXAMPLES
        fromdir = $${PWD}/EXAMPLES
        todir ~= s,/,\\,g
        fromdir ~= s,/,\\,g
        QMAKE_POST_LINK = $$quote(cmd /c xcopy \"$${fromdir}\" \"$${todir}\" /y /e /i$$escape_expand(\n\t))
      }

linux-g++ {
   #todir = $${OUT_PWD}/..
   todir = $${OUT_PWD}
   fromdir = $${PWD}/EXAMPLES
   QMAKE_POST_LINK = $$quote(cp -rf \"$${fromdir}\" \"$${todir}\"$$escape_expand(\n\t))
}

unix {
   #todir = $${OUT_PWD}/..
   todir = $${OUT_PWD}
   fromdir = $${PWD}/EXAMPLES
   QMAKE_POST_LINK = $$quote(cp -rf \"$${fromdir}\" \"$${todir}\"$$escape_expand(\n\t))
}
#------------

DISTFILES +=
