#--------------ANTS2--------------
ANTS2_MAJOR = 4
ANTS2_MINOR = 7

#Optional libraries
CONFIG += ants2_cuda        #enable CUDA support - need NVIDIA GPU and drivers (CUDA toolkit) installed!
CONFIG += ants2_flann       #enable FLANN (fast neighbour search) library
CONFIG += ants2_fann        #enables FANN (fast neural network) library
CONFIG += ants2_eigen3      #use Eigen3 library instead of ROOT for linear algebra
CONFIG += ants2_RootServer  #enable cern CERN ROOT html server
CONFIG += ants2_Python      #enable Python scripting

DEBUG_VERBOSITY = 1          # 0 - debug messages suppressed, 1 - normal, 2 - normal + file/line information
                             # after a change, qmake and rebuild (or qmake + make any change in main.cpp to trigger recompilation)

#---CERN ROOT---
win32 {
     INCLUDEPATH += c:/root/include
     LIBS += -Lc:/root/lib/ -llibCore -llibCint -llibRIO -llibNet -llibHist -llibGraf -llibGraf3d -llibGpad -llibTree -llibRint -llibPostscript -llibMatrix -llibPhysics -llibRint -llibMathCore -llibGeom -llibGeomPainter -llibGeomBuilder -llibMathMore -llibMinuit2 -llibThread -llibSpectrum
     ants2_RootServer {LIBS += -llibRHTTP}
}
linux-g++ || unix {
     INCLUDEPATH += $$system(root-config --incdir)
     LIBS += $$system(root-config --libs) -lGeom -lGeomPainter -lGeomBuilder -lMathMore -lMinuit2 -lSpectrum
     ants2_RootServer {LIBS += -llibRHTTP}
}
#-----------

#---EIGEN---
ants2_eigen3 {
     DEFINES += USE_EIGEN
     CONFIG += ants2_matrix #if enabled - use matrix algebra for TP splines - UNDER DEVELOPMENT

     win32 { INCLUDEPATH += C:/eigen3 }
     linux-g++ || unix { INCLUDEPATH += /usr/include/eigen3 }

     #advanced options:
     DEFINES += NEWFIT #if enabled, use advanced fitting class (non-negative LS, non-decreasing LS, hole-plugging, etc.)  

     SOURCES += SplineLibrary/bs3fit.cpp \
                SplineLibrary/tps3fit.cpp \


     HEADERS += SplineLibrary/bs3fit.h \
                SplineLibrary/tps3fit.h
}
ants2_matrix { # use matrix algebra for TP splines
    DEFINES += TPS3M
    SOURCES += SplineLibrary/tpspline3m.cpp \
               SplineLibrary/tpspline3d.cpp \
               SplineLibrary/tps3dfit.cpp \
               modules/lrf_v2/lrfxyz.cpp

    HEADERS += SplineLibrary/tpspline3m.h \
               SplineLibrary/tpspline3d.h \
               SplineLibrary/tps3dfit.h \
               modules/lrf_v2/lrfxyz.h

} else {
    SOURCES += SplineLibrary/tpspline3.cpp
    HEADERS += SplineLibrary/tpspline3.h
} 
#----------

#---FLANN---
ants2_flann {
     DEFINES += ANTS_FLANN

     win32 {
        LIBS += -LC:/FLANN/lib -lflann
        INCLUDEPATH += C:/FLANN/include
     }

    HEADERS += modules/nnmoduleclass.h
    SOURCES += modules/nnmoduleclass.cpp

    HEADERS += scriptmode/ainterfacetoknnscript.h
    SOURCES += scriptmode/ainterfacetoknnscript.cpp
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

     #main module
    HEADERS += modules/neuralnetworksmodule.h
    SOURCES += modules/neuralnetworksmodule.cpp
      #gui only
    HEADERS += gui/neuralnetworkswindow.h
    SOURCES += gui/neuralnetworkswindow.cpp
      #interface to script module
    HEADERS += scriptmode/ainterfacetoannscript.h
    SOURCES += scriptmode/ainterfacetoannscript.cpp
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

#---ROOT server---
ants2_RootServer{
  DEFINES += USE_ROOT_HTML
}
#----------

#---Python---
ants2_Python{
    DEFINES += __USE_ANTS_PYTHON__

    #http://pythonqt.sourceforge.net/
    win32:{
            INCLUDEPATH += c:/Python33/include
            LIBS += -Lc:/Python33/libs -lPython33

            INCLUDEPATH += C:/PythonQt3.2/src
            INCLUDEPATH += C:/PythonQt3.2/extensions/PythonQt_QtAll

            LIBS += -LC:/PythonQt3.2 -lPythonQt_QtAll-Qt5-Python333 -lPythonQt-Qt5-Python333
    }
    linux-g++ || unix {
            INCLUDEPATH += $$system(python-config --includes)
            LIBS += $$system(python-config --libs)

            INCLUDEPATH += usr/PythonQt3.2/src
            INCLUDEPATH += usr/PythonQt3.2/extensions/PythonQt_QtAll

            LIBS += -Lusr/PythonQt3.2 -lPythonQt_QtAll-Qt5-Python27 -lPythonQt-Qt5-Python27
    }

    HEADERS += scriptmode/apythonscriptmanager.h
    SOURCES += scriptmode/apythonscriptmanager.cpp

    SOURCES += gui/MainWindowTools/pythonscript.cpp
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
    modules/eventsdataclass.cpp \
    modules/dynamicpassiveshandler.cpp \
    modules/processorclass.cpp \
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
    scriptmode/ascriptexample.cpp \
    scriptmode/ascriptexampledatabase.cpp \
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
    scriptmode/ainterfacetophotonscript.cpp \
    common/aphotonhistorylog.cpp \
    common/amonitor.cpp \
    common/aroothistappenders.cpp \
    gui/MainWindowTools/amonitordelegateform.cpp \
    common/amonitorconfig.cpp \
    common/apeakfinder.cpp \
    common/amaterialcomposition.cpp \
    gui/aelementandisotopedelegates.cpp \
    gui/amatparticleconfigurator.cpp \
    common/aneutroninteractionelement.cpp \
    gui/aneutronreactionsconfigurator.cpp \
    gui/aneutronreactionwidget.cpp \
    gui/aneutroninfodialog.cpp \
    scriptmode/ainterfacetodeposcript.cpp \
    scriptmode/ainterfacetomessagewindow.cpp \
    scriptmode/coreinterfaces.cpp \
    scriptmode/localscriptinterfaces.cpp \
    scriptmode/histgraphinterfaces.cpp \
    gui/GraphWindowTools/atoolboxscene.cpp \
    scriptmode/ainterfacetomultithread.cpp \
    scriptmode/ascriptmessengerdialog.cpp \
    scriptmode/arootgraphrecord.cpp \
    scriptmode/aroothistrecord.cpp \
    common/arootobjcollection.cpp \
    common/arootobjbase.cpp \
    common/acalibratorsignalperphel.cpp \
    modules/areconstructionmanager.cpp \
    scriptmode/ajavascriptmanager.cpp \
    scriptmode/ascriptmanager.cpp \
    common/apm.cpp \
    modules/apmhub.cpp \
    common/apmtype.cpp \
    scriptmode/ainterfacetoguiscript.cpp \
    modules/aoneevent.cpp \
    scriptmode/ainterfacetottree.cpp \
    common/aroottreerecord.cpp \
    common/atextedit.cpp \
    gui/alineedit.cpp

HEADERS  += common/CorrelationFilters.h \
    common/jsonparser.h \
    common/reconstructionsettings.h \
    common/generalsimsettings.h \
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
    modules/particlesourcesclass.h \    
    modules/flatfield.h \    
    modules/sensorlrfs.h \
    modules/eventsdataclass.h \
    modules/dynamicpassiveshandler.h \
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
    scriptmode/ascriptexample.h \
    scriptmode/ascriptexampledatabase.h \
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
    scriptmode/ainterfacetophotonscript.h \
    common/aphotonhistorylog.h \
    common/amonitor.h \
    common/aroothistappenders.h \
    gui/MainWindowTools/amonitordelegateform.h \
    common/amonitorconfig.h \
    common/apeakfinder.h \
    common/amaterialcomposition.h \
    gui/aelementandisotopedelegates.h \
    gui/amatparticleconfigurator.h \
    common/aneutroninteractionelement.h \
    gui/aneutronreactionsconfigurator.h \
    gui/aneutronreactionwidget.h \
    gui/aneutroninfodialog.h \
    scriptmode/ainterfacetodeposcript.h \
    scriptmode/ainterfacetomessagewindow.h \
    scriptmode/coreinterfaces.h \
    scriptmode/localscriptinterfaces.h \
    scriptmode/histgraphinterfaces.h \
    common/amessageoutput.h \
    gui/GraphWindowTools/atoolboxscene.h \
    scriptmode/ainterfacetomultithread.h \
    scriptmode/ascriptinterfacefactory.h \
    scriptmode/ascriptmessengerdialog.h \
    scriptmode/arootgraphrecord.h \
    common/arootobjcollection.h \
    scriptmode/aroothistrecord.h \
    common/arootobjbase.h \
    common/acalibratorsignalperphel.h \
    modules/areconstructionmanager.h \
    scriptmode/ajavascriptmanager.h \
    scriptmode/ascriptmanager.h \
    common/apm.h \
    modules/apmhub.h \
    common/apmtype.h \
    scriptmode/ainterfacetoguiscript.h \
    modules/aoneevent.h \
    scriptmode/ainterfacetottree.h \
    common/aroottreerecord.h \
    common/atextedit.h \
    gui/alineedit.h

# --- SIM ---
ants2_SIM {
    DEFINES += SIM

    SOURCES += common/asimulationstatistics.cpp \
    modules/primaryparticletracker.cpp \
    modules/s1_generator.cpp \
    modules/photon_generator.cpp \
    modules/s2_generator.cpp \
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
    gui/MainWindowTools/globalscript.cpp \
    gui/MainWindowTools/aslablistwidget.cpp \
    gui/MainWindowTools/arootlineconfigurator.cpp \
    gui/MainWindowTools/ageotreewidget.cpp \
    gui/MainWindowTools/ashapehelpdialog.cpp \
    gui/MainWindowTools/agridelementdialog.cpp \
    gui/ascriptexampleexplorer.cpp \
    gui/MainWindowTools/mainwindowjson.cpp \
    gui/ReconstructionWindowTools/reconstructionwindowguiupdates.cpp \
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
    gui/MainWindowTools/aslablistwidget.h \
    gui/MainWindowTools/arootlineconfigurator.h \
    gui/MainWindowTools/ageotreewidget.h \
    gui/MainWindowTools/ashapehelpdialog.h \
    gui/MainWindowTools/agridelementdialog.h \
    gui/ascriptexampleexplorer.h \
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
INCLUDEPATH += gui/GraphWindowTools
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

#---Optimization of compilation---
win32 {
  #uncomment the next two lines to disable optimization during compilation. It will drastically shorten compilation time, but there are performance loss, especially strong for LRF computation
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE *= -Od
}
linux-g++ || unix {
    QMAKE_CXXFLAGS += -march=native
}
#------------

#---Windows-specific compilation mode and warning suppression
win32 {
  #CONFIG   += console                  #enable to add standalone console for Windows
  DEFINES  += _CRT_SECURE_NO_WARNINGS   #disable microsoft spam
  #DEFINES += WINDOWSBIN                #enable for compilation in Windows binary-only mode
}
#------------

#---Additional defines---
DEFINES += ANTS2_MINOR=\"$$ANTS2_MINOR\"
DEFINES += ANTS2_MAJOR=\"$$ANTS2_MAJOR\"
DEFINES += DEBUG_VERBOSITY=\"$$DEBUG_VERBOSITY\"

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

FORMS += \
    gui/MainWindowTools/amonitordelegateform.ui \
    gui/amatparticleconfigurator.ui \
    gui/aneutronreactionsconfigurator.ui \
    gui/aneutronreactionwidget.ui \
    gui/aneutroninfodialog.ui
