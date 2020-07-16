#--------------ANTS2--------------
ANTS2_MAJOR = 4
ANTS2_MINOR = 31

# !!! You may need to modify path for CERN ROOT, see #---CERN ROOT--- section below

#Optional libraries
#CONFIG += ants2_cuda        #enable CUDA support - need NVIDIA GPU and drivers (CUDA toolkit) installed!
#CONFIG += ants2_flann       #enable FLANN (fast neighbour search) library: see https://github.com/mariusmuja/flann
#CONFIG += ants2_fann        #enables FANN (fast neural network) library: see https://github.com/libfann/fann
CONFIG += ants2_eigen3      #use Eigen3 library instead of ROOT for linear algebra - highly recommended! Installation requires only to copy files!
CONFIG += ants2_RootServer  #enable cern CERN ROOT html server
CONFIG += ants2_Python      #enable Python scripting
CONFIG += ants2_NCrystal    #enable NCrystal library (neutron scattering): see https://github.com/mctools/ncrystal
CONFIG += ants2_jsroot       #enables JSROOT visualisation at GeometryWindow. Automatically enables ants2_RootServer

#In effect ONLY for the Docker version:
ants2_docker {
    CONFIG += ANTS_DOCKER
    CONFIG += ants2_flann
    CONFIG += ants2_fann
    CONFIG += ants2_RootServer
    CONFIG += ants2_Python
    CONFIG += ants2_NCrystal
}

#---CERN ROOT---
linux-g++ || unix {
     INCLUDEPATH += $$system(root-config --incdir)
     LIBS += $$system(root-config --libs) -lGeom -lGeomPainter -lGeomBuilder -lMinuit2 -lSpectrum -ltbb
     ants2_RootServer {LIBS += -lRHTTP  -lXMLIO}
}
win32 {
     INCLUDEPATH += c:/root/include
     LIBS += -Lc:/root/lib/ -llibCore -llibRIO -llibNet -llibHist -llibGraf -llibGraf3d -llibGpad -llibTree -llibRint -llibPostscript -llibMatrix -llibPhysics -llibMathCore -llibGeom -llibGeomPainter -llibGeomBuilder -llibMinuit2 -llibThread -llibSpectrum
     ants2_RootServer {LIBS += -llibRHTTP}
}
#-----------

#---EIGEN---
ants2_eigen3 {
     DEFINES += USE_EIGEN
     CONFIG += ants2_matrix #if enabled - use matrix algebra for TP splines - UNDER DEVELOPMENT

     linux-g++ || unix { INCLUDEPATH += /usr/include/eigen3 }
     win32 { INCLUDEPATH += C:/eigen3 }

     #advanced options:
     DEFINES += NEWFIT #if enabled, use advanced fitting class (non-negative LS, non-decreasing LS, hole-plugging, etc.)  

     SOURCES += SplineLibrary/bs3fit.cpp \
                SplineLibrary/tps3fit.cpp \
                SplineLibrary/curvefit.cpp

     HEADERS += SplineLibrary/bs3fit.h \
                SplineLibrary/tps3fit.h \
                SplineLibrary/curvefit.h
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

    linux-g++ || unix {
        LIBS += -lflann
        ants2_docker{ LIBS += -llz4 }
    }
    win32 {
       LIBS += -LC:/FLANN/lib -lflann
       INCLUDEPATH += C:/FLANN/include
    }

    HEADERS += modules/nnmoduleclass.h
    SOURCES += modules/nnmoduleclass.cpp
      #interface to script module
    HEADERS += scriptmode/aknn_si.h
    SOURCES += scriptmode/aknn_si.cpp
}
#----------

#---FANN---
ants2_fann {
    DEFINES += ANTS_FANN

    linux-g++ || unix {
        LIBS += -L/usr/local/lib -lfann
    }
    win32 {
        LIBS += -Lc:/FANN-2.2.0-Source/bin/ -lfannfloat
        INCLUDEPATH += c:/FANN-2.2.0-Source/src/include
        DEFINES += NOMINMAX
    }

     #main module
    HEADERS += modules/neuralnetworksmodule.h
    SOURCES += modules/neuralnetworksmodule.cpp
      #gui only
    HEADERS += gui/neuralnetworkswindow.h
    SOURCES += gui/neuralnetworkswindow.cpp
      #interface to script module
    HEADERS += scriptmode/aann_si.h
    SOURCES += scriptmode/aann_si.cpp
}
#---------

#---CUDA---
ants2_cuda {
    DEFINES += __USE_ANTS_CUDA__

    #Path to CUDA toolkit:
    linux-g++ || unix { CUDA_DIR = $$system(which nvcc | sed 's,/bin/nvcc$,,') }
    win32 { CUDA_DIR = "C:/cuda/toolkit" }        # AVOID SPACES IN THE PATH during toolkit installation!

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

#---Python---
ants2_Python{
    DEFINES += __USE_ANTS_PYTHON__

    #http://pythonqt.sourceforge.net/ or https://github.com/Orochimarufan/PythonQt
    #for PythonQt installation see instructions in PythonQtInstall.txt in the root of ANTS2 on GitHub
    linux-g++ || unix {
            ants2_docker { 
                LIBS += $$system(python3-config --libs)
                QMAKE_CXXFLAGS += $$system(python3-config --includes)

                INCLUDEPATH += /usr/include/PythonQt5/
                LIBS += -lPythonQt-Qt5-Python3.6
            } else {
                LIBS += $$system(python3-config --libs)
                QMAKE_CXXFLAGS += $$system(python3-config --includes)

                INCLUDEPATH += /usr/include/PythonQt5/
                LIBS += -lPythonQt-Qt5-Python3.6
            }
    }
    win32:{
            INCLUDEPATH += 'C:/Program Files (x86)/Microsoft Visual Studio/Shared/Python36_86/include'
            LIBS += -L'C:/Program Files (x86)/Microsoft Visual Studio/Shared/Python36_86/libs' -lPython36

            INCLUDEPATH += C:/PythonQt3.2/src
            LIBS += -LC:/PythonQt3.2/lib -lPythonQt
    }

    HEADERS += scriptmode/apythonscriptmanager.h
    SOURCES += scriptmode/apythonscriptmanager.cpp

    SOURCES += gui/MainWindowTools/pythonscript.cpp
}
#----------

#---NCrystal---
#see https://github.com/mctools/ncrystal
ants2_NCrystal{
    DEFINES += __USE_ANTS_NCRYSTAL__

    linux-g++ || unix {
            ants2_docker {
                INCLUDEPATH += /usr/local/include/NCrystal/
                LIBS += -L/usr/local/lib/
                LIBS += -lNCrystal

            } else {
                INCLUDEPATH += /usr/local/include/NCrystal
                LIBS += -L/usr/local/lib/
                LIBS += -lNCrystal
            }
    }
    win32:{
            INCLUDEPATH += C:/NCrystal/ncrystal_core/include
            LIBS += -LC:/NCrystal/lib -lNCrystal
    }

    SOURCES += common/arandomgenncrystal.cpp
    HEADERS += common/arandomgenncrystal.h
}
#----------

#---JSROOT viewer---
ants2_jsroot{
    DEFINES += __USE_ANTS_JSROOT__
    QT      += webenginewidgets
    CONFIG  += ants2_RootServer
}
#----------

#---ROOT server---
ants2_RootServer{
  DEFINES += USE_ROOT_HTML

    SOURCES += Net/aroothttpserver.cpp
    HEADERS += Net/aroothttpserver.h
}
#----------

DEBUG_VERBOSITY = 1          # 0 - debug messages suppressed, 1 - normal, 2 - normal + file/line information
                             # after a change, qmake and rebuild (or qmake + make any change in main.cpp to trigger recompilation)

CONFIG += ants2_GUI         #if disabled, GUI is not compiled
CONFIG += ants2_SIM         #if disabled, simulation-related modules are not compiled

#Can be used as command line option to force-disable GUI
Headless {
message("--> Compiling without GUI")
CONFIG -= ants2_GUI
}

#---ANTS2 files---
SOURCES += main.cpp \
    common/jsonparser.cpp \
    common/CorrelationFilters.cpp \
    common/reconstructionsettings.cpp \
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
    OpticalOverrides/aopticaloverride.cpp \
    modules/detectorclass.cpp \
    modules/eventsdataclass.cpp \
    modules/dynamicpassiveshandler.cpp \
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
    scriptmode/scriptminimizer.cpp \
    scriptmode/ascriptexample.cpp \
    scriptmode/ascriptexampledatabase.cpp \
    common/aslab.cpp \
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
    common/ascriptvaluecopier.cpp \
    common/acustomrandomsampling.cpp \
    common/amaterial.cpp \
    common/aparticle.cpp \
    modules/amaterialparticlecolection.cpp\
    common/ascriptvalueconverter.cpp \
    common/ainternetbrowser.cpp \
    Net/anetworkmodule.cpp \
    common/aphotonhistorylog.cpp \
    common/amonitor.cpp \
    common/aroothistappenders.cpp \
    common/amonitorconfig.cpp \
    common/apeakfinder.cpp \
    common/amaterialcomposition.cpp \
    common/aneutroninteractionelement.cpp \
    scriptmode/localscriptinterfaces.cpp \
    scriptmode/histgraphinterfaces.cpp \
    scriptmode/arootgraphrecord.cpp \
    scriptmode/aroothistrecord.cpp \
    common/arootobjcollection.cpp \
    common/arootobjbase.cpp \
    common/acalibratorsignalperphel.cpp \
    Reconstruction/areconstructionmanager.cpp \
    scriptmode/ajavascriptmanager.cpp \
    scriptmode/ascriptmanager.cpp \
    common/apm.cpp \
    modules/apmhub.cpp \
    common/apmtype.cpp \
    modules/aoneevent.cpp \
    common/aroottreerecord.cpp \
    Net/awebsocketsessionserver.cpp \
    Net/awebsocketstandalonemessanger.cpp \
    Net/awebsocketsession.cpp \
    common/agammarandomgenerator.cpp \
    Net/agridrunner.cpp \
    Net/aremoteserverrecord.cpp \
    common/atrackbuildoptions.cpp \
    OpticalOverrides/aopticaloverridescriptinterface.cpp \
    OpticalOverrides/ascriptopticaloverride.cpp \
    common/atracerstateful.cpp \
    OpticalOverrides/fsnpopticaloverride.cpp \
    OpticalOverrides/awaveshifteroverride.cpp \
    OpticalOverrides/spectralbasicopticaloverride.cpp \
    OpticalOverrides/abasicopticaloverride.cpp \
    common/aglobalsettings.cpp \
    common/aparticlesourcerecord.cpp \
    Simulation/ascriptparticlegenerator.cpp \
    Simulation/afileparticlegenerator.cpp \
    Simulation/aparticlerecord.cpp \
    Simulation/asourceparticlegenerator.cpp \
    common/aisotopeabundancehandler.cpp \
    common/aisotope.cpp \
    common/achemicalelement.cpp \
    Simulation/ag4simulationsettings.cpp \
    scriptmode/asim_si.cpp \
    scriptmode/amath_si.cpp \
    scriptmode/atree_si.cpp \
    scriptmode/aphoton_si.cpp \
    scriptmode/acore_si.cpp \
    scriptmode/ageo_si.cpp \
    scriptmode/aserver_si.cpp \
    scriptmode/aweb_si.cpp \
    scriptmode/athreads_si.cpp \
    scriptmode/aparticlegenerator_si.cpp \
    scriptmode/agstyle_si.cpp \
    scriptmode/aconfig_si.cpp \
    scriptmode/aevents_si.cpp \
    scriptmode/arec_si.cpp \
    scriptmode/apms_si.cpp \
    scriptmode/alrf_si.cpp \
    common/aeventtrackingrecord.cpp \
    scriptmode/apthistory_si.cpp \
    Simulation/atrackinghistorycrawler.cpp \
    Simulation/aparticletracker.cpp \
    common/aexternalprocesshandler.cpp \
    Simulation/alogsandstatisticsoptions.cpp \
    Reconstruction/areconstructionworker.cpp \
    common/ahistogram.cpp \
    modules/apmdummystructure.cpp \
    SplineLibrary/Spline123/profileHist.cpp \
    SplineLibrary/Spline123/json11.cpp \
    SplineLibrary/Spline123/bspline123d.cpp \
    SplineLibrary/Spline123/bsfit123.cpp \
    scriptmode/afarm_si.cpp

HEADERS  += common/CorrelationFilters.h \
    common/jsonparser.h \
    common/reconstructionsettings.h \
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
    OpticalOverrides/aopticaloverride.h \
    modules/detectorclass.h \
    modules/flatfield.h \
    modules/sensorlrfs.h \
    modules/eventsdataclass.h \
    modules/dynamicpassiveshandler.h \
    modules/manifesthandling.h \
    modules/apmgroupsmanager.h \
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
    scriptmode/scriptminimizer.h \
    scriptmode/ascriptexample.h \
    scriptmode/ascriptexampledatabase.h \
    scriptmode/ascriptinterface.h \
    modules/asandwich.h \
    common/aslab.h \
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
    common/ascriptvaluecopier.h \
    common/acustomrandomsampling.h \
    common/amaterial.h \
    common/aparticle.h \
    modules/amaterialparticlecolection.h\
    common/ascriptvalueconverter.h \
    common/ainternetbrowser.h \
    Net/anetworkmodule.h \
    SplineLibrary/eiquadprog.hpp \
    common/aphotonhistorylog.h \
    common/amonitor.h \
    common/aroothistappenders.h \
    common/amonitorconfig.h \
    common/apeakfinder.h \
    common/amaterialcomposition.h \
    common/aneutroninteractionelement.h \
    scriptmode/localscriptinterfaces.h \
    scriptmode/histgraphinterfaces.h \
    common/amessageoutput.h \
    scriptmode/ascriptinterfacefactory.h \
    scriptmode/arootgraphrecord.h \
    common/arootobjcollection.h \
    scriptmode/aroothistrecord.h \
    common/arootobjbase.h \
    common/acalibratorsignalperphel.h \
    Reconstruction/areconstructionmanager.h \
    scriptmode/ajavascriptmanager.h \
    scriptmode/ascriptmanager.h \
    common/apm.h \
    modules/apmhub.h \
    common/apmtype.h \
    modules/aoneevent.h \
    common/aroottreerecord.h \
    Net/awebsocketsessionserver.h \
    Net/awebsocketstandalonemessanger.h \
    Net/awebsocketsession.h \
    Net/agridrunner.h \
    Net/aremoteserverrecord.h \
    common/atrackbuildoptions.h \
    OpticalOverrides/aopticaloverridescriptinterface.h \
    OpticalOverrides/ascriptopticaloverride.h \
    common/atracerstateful.h \
    OpticalOverrides/fsnpopticaloverride.h \
    OpticalOverrides/awaveshifteroverride.h \
    OpticalOverrides/spectralbasicopticaloverride.h \
    OpticalOverrides/abasicopticaloverride.h \
    common/aglobalsettings.h \
    common/aparticlesourcerecord.h \
    Simulation/aparticlegun.h \
    Simulation/ascriptparticlegenerator.h \
    Simulation/afileparticlegenerator.h \
    Simulation/aparticlerecord.h \
    Simulation/asourceparticlegenerator.h \
    common/aisotopeabundancehandler.h \
    common/aisotope.h \
    common/achemicalelement.h \
    Simulation/ag4simulationsettings.h \
    scriptmode/asim_si.h \
    scriptmode/amath_si.h \
    scriptmode/atree_si.h \
    scriptmode/aphoton_si.h \
    scriptmode/acore_si.h \
    scriptmode/ageo_si.h \
    scriptmode/aserver_si.h \
    scriptmode/aweb_si.h \
    scriptmode/athreads_si.h \
    scriptmode/aparticlegenerator_si.h \
    scriptmode/agstyle_si.h \
    scriptmode/aconfig_si.h \
    scriptmode/aevents_si.h \
    scriptmode/arec_si.h \
    scriptmode/apms_si.h \
    scriptmode/alrf_si.h \
    common/aeventtrackingrecord.h \
    scriptmode/apthistory_si.h \
    Simulation/atrackinghistorycrawler.h \
    Simulation/aparticletracker.h \
    common/aexternalprocesshandler.h \
    Simulation/alogsandstatisticsoptions.h \
    Reconstruction/afunctorbase.h \
    Reconstruction/areconstructionworker.h \
    common/ahistogram.h \
    modules/apmanddummy.h \
    modules/apmdummystructure.h \
    SplineLibrary/Spline123/profileHist.h \
    SplineLibrary/Spline123/json11.hpp \
    SplineLibrary/Spline123/eiquadprog.hpp \
    SplineLibrary/Spline123/bspline123d.h \
    SplineLibrary/Spline123/bsfit123.h \
    scriptmode/afarm_si.h

# --- SIM ---
ants2_SIM {
    DEFINES += SIM

    SOURCES += Simulation/aphoton.cpp \
    Simulation/asimulationstatistics.cpp \
    Simulation/s1_generator.cpp \
    Simulation/photon_generator.cpp \
    Simulation/s2_generator.cpp \
    OpticalOverrides/phscatclaudiomodel.cpp \
    OpticalOverrides/scatteronmetal.cpp \
    Simulation/aphotontracer.cpp \
    Simulation/acompton.cpp \
    Simulation/ageometrytester.cpp \
    Simulation/atrackingdataimporter.cpp \
    Simulation/anoderecord.cpp \
    Simulation/asimulationmanager.cpp \
    Simulation/asimulatorrunner.cpp \
    Simulation/asimulator.cpp \
    Simulation/apointsourcesimulator.cpp \
    Simulation/amaterialloader.cpp \
    Simulation/aparticlesourcesimulator.cpp \
    Simulation/asaveparticlestofilesettings.cpp \
    Simulation/aphotonsimsettings.cpp \
    Simulation/ageneralsimsettings.cpp \
    Simulation/asimsettings.cpp \
    Simulation/aparticlesimsettings.cpp \
    Simulation/aphotonnodedistributor.cpp \
    Simulation/a3dposprob.cpp

    HEADERS  += Simulation/aphoton.h \
    Simulation/asimulationstatistics.h \
    Simulation/agridelementrecord.h \
    Simulation/ageomarkerclass.h \
    Simulation/atrackrecords.h \
    Simulation/dotstgeostruct.h \
    Simulation/aenergydepositioncell.h \
    Simulation/ahistoryrecords.h \
    Simulation/s1_generator.h \
    Simulation/photon_generator.h \
    Simulation/s2_generator.h \
    OpticalOverrides/phscatclaudiomodel.h \
    OpticalOverrides/scatteronmetal.h \
    Simulation/aphotontracer.h \
    Simulation/acompton.h \
    Simulation/ageometrytester.h \
    Simulation/atrackingdataimporter.h \
    Simulation/anoderecord.h \
    Simulation/asimulationmanager.h \
    Simulation/asimulatorrunner.h \
    Simulation/asimulator.h \
    Simulation/apointsourcesimulator.h \
    Simulation/amaterialloader.h \
    Simulation/aparticlesourcesimulator.h \
    Simulation/asaveparticlestofilesettings.h \
    Simulation/aphotonsimsettings.h \
    Simulation/ageneralsimsettings.h \
    Simulation/asimsettings.h \
    Simulation/aparticlesimsettings.h \
    Simulation/aphotonnodedistributor.h \
    Simulation/a3dposprob.h
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
    gui/alrfdraw.cpp \
    gui/MainWindowTools/arootmarkerconfigurator.cpp \
    gui/ahighlighters.cpp \
    modules/lrf_v3/gui/atpspline3widget.cpp \
    modules/lrf_v3/gui/avladimircompressionwidget.cpp \
    gui/MainWindowTools/amonitordelegateform.cpp \
    gui/aelementandisotopedelegates.cpp \
    gui/amatparticleconfigurator.cpp \
    gui/aneutronreactionsconfigurator.cpp \
    gui/aneutronreactionwidget.cpp \
    gui/aneutroninfodialog.cpp \
    gui/GraphWindowTools/atoolboxscene.cpp \
    scriptmode/ascriptmessengerdialog.cpp \
    gui/atextedit.cpp \
    gui/alineedit.cpp \
    gui/awebsocketserverdialog.cpp \
    common/acollapsiblegroupbox.cpp \
    gui/MainWindowTools/slabdelegate.cpp \
    gui/aremotewindow.cpp \
    gui/MainWindowTools/atrackdrawdialog.cpp \
    gui/aserverdelegate.cpp \
    gui/MainWindowTools/aopticaloverridedialog.cpp \
    gui/MainWindowTools/aopticaloverridetester.cpp \
    scriptmode/amsg_si.cpp \
    scriptmode/agui_si.cpp \
    scriptmode/ageowin_si.cpp \
    scriptmode/agraphwin_si.cpp \
    scriptmode/aoutwin_si.cpp \
    gui/MainWindowTools/aparticlesourcedialog.cpp \
    gui/MainWindowTools/alogconfigdialog.cpp \
    gui/GraphWindowTools/abasketitem.cpp \
    gui/GraphWindowTools/abasketmanager.cpp \
    gui/GraphWindowTools/adrawexplorerwidget.cpp \
    gui/GraphWindowTools/adrawobject.cpp \
    gui/GraphWindowTools/abasketlistwidget.cpp \
    gui/GraphWindowTools/alegenddialog.cpp \
    gui/MainWindowTools/aroottextconfigurator.cpp \
    gui/aproxystyle.cpp \
    gui/GraphWindowTools/aaxesdialog.cpp \
    gui/GraphWindowTools/atextpavedialog.cpp \
    gui/GraphWindowTools/alinemarkerfilldialog.cpp \
    gui/GraphWindowTools/arootcolorselectordialog.cpp \
    gui/GraphWindowTools/adrawtemplate.cpp \
    gui/GraphWindowTools/atemplateselectiondialog.cpp \
    gui/GraphWindowTools/atemplateselectionrecord.cpp \
    gui/amateriallibrarybrowser.cpp \
    gui/DAWindowTools/amaterialloaderdialog.cpp \
    gui/ageant4configdialog.cpp \
    gui/MainWindowTools/asaveparticlestofiledialog.cpp \
    gui/aguiwindow.cpp

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
    gui/alrfdraw.h \
    gui/MainWindowTools/arootmarkerconfigurator.h \
    gui/ahighlighters.h \
    modules/lrf_v3/gui/atpspline3widget.h \
    modules/lrf_v3/gui/avladimircompressionwidget.h \
    gui/MainWindowTools/amonitordelegateform.h \
    gui/aelementandisotopedelegates.h \
    gui/amatparticleconfigurator.h \
    gui/aneutronreactionsconfigurator.h \
    gui/aneutronreactionwidget.h \
    gui/aneutroninfodialog.h \
    gui/GraphWindowTools/atoolboxscene.h \
    scriptmode/ascriptmessengerdialog.h \
    gui/atextedit.h \
    gui/alineedit.h \
    gui/MainWindowTools/slabdelegate.h \
    common/acollapsiblegroupbox.h \
    gui/awebsocketserverdialog.h \
    gui/aremotewindow.h \
    gui/MainWindowTools/atrackdrawdialog.h \
    gui/aserverdelegate.h \
    gui/MainWindowTools/aopticaloverridedialog.h \
    gui/MainWindowTools/aopticaloverridetester.h \
    scriptmode/amsg_si.h \
    scriptmode/agui_si.h \
    scriptmode/ageowin_si.h \
    scriptmode/agraphwin_si.h \
    scriptmode/aoutwin_si.h \
    gui/MainWindowTools/aparticlesourcedialog.h \
    gui/MainWindowTools/alogconfigdialog.h \
    gui/GraphWindowTools/adrawobject.h \
    gui/GraphWindowTools/abasketitem.h \
    gui/GraphWindowTools/abasketmanager.h \
    gui/GraphWindowTools/adrawexplorerwidget.h \
    gui/GraphWindowTools/abasketlistwidget.h \
    gui/GraphWindowTools/alegenddialog.h \
    gui/MainWindowTools/aroottextconfigurator.h \
    gui/aproxystyle.h \
    gui/GraphWindowTools/aaxesdialog.h \
    gui/GraphWindowTools/atextpavedialog.h \
    gui/GraphWindowTools/alinemarkerfilldialog.h \
    gui/GraphWindowTools/arootcolorselectordialog.h \
    gui/GraphWindowTools/adrawtemplate.h \
    gui/GraphWindowTools/atemplateselectiondialog.h \
    gui/GraphWindowTools/atemplateselectionrecord.h \
    gui/amateriallibrarybrowser.h \
    gui/DAWindowTools/amaterialloaderdialog.h \
    gui/ageant4configdialog.h \
    gui/MainWindowTools/asaveparticlestofiledialog.h \
    gui/aguiwindow.h

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
    modules/lrf_v3/gui/alrfwindow.ui \
    gui/MainWindowTools/amonitordelegateform.ui \
    gui/amatparticleconfigurator.ui \
    gui/aneutronreactionsconfigurator.ui \
    gui/aneutronreactionwidget.ui \
    gui/aneutroninfodialog.ui \
    gui/awebsocketserverdialog.ui \
    gui/aremotewindow.ui \
    gui/MainWindowTools/atrackdrawdialog.ui \
    gui/MainWindowTools/aopticaloverridedialog.ui \
    gui/MainWindowTools/aopticaloverridetester.ui \
    gui/MainWindowTools/aparticlesourcedialog.ui

INCLUDEPATH += gui
INCLUDEPATH += gui/RasterWindow
INCLUDEPATH += gui/ReconstructionWindowTools
INCLUDEPATH += modules/lrf_v3/gui
INCLUDEPATH += gui/GraphWindowTools
INCLUDEPATH += gui/DAWindowTools
}

INCLUDEPATH += gui/MainWindowTools

INCLUDEPATH += SplineLibrary
INCLUDEPATH += OpticalOverrides
INCLUDEPATH += modules
INCLUDEPATH += modules/lrf_v2
INCLUDEPATH += modules/lrf_v3
INCLUDEPATH += common
INCLUDEPATH += scriptmode
INCLUDEPATH += Net
INCLUDEPATH += Simulation
INCLUDEPATH += Reconstruction

RESOURCES += \
    Resources.qrc
#-------------

#---Qt kitchen---
QT += core
ants2_GUI {
    QT += gui
} else {
    QT -= gui
}
# couldn't get of widgets yet due to funny compile erors - VS
QT += widgets
QT += websockets
QT += script #scripts support
win32:QT += winextras  #used in windownavigator only

CONFIG += c++11

TARGET = ants2
TEMPLATE = app

RC_FILE = myapp.rc
#------------

# The next define allows the core script interface to launch external processes. Enable only if you know what are you doing
#DEFINES += _ALLOW_LAUNCH_EXTERNAL_PROCESS_

#---Optimization of compilation---
win32 {
  #when the next two lines are NOT commented, optimization during compilation is disabled. It will drastically shorten compilation time on MSVC2013, but there are performance loss, especially strong for LRF computation
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
  DEFINES  += _LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER #disables warning C4068: unknown pragma
  #DEFINES += WINDOWSBIN                #enable for compilation in Windows binary-only mode
}
#------------

#---Additional defines---
DEFINES += ANTS2_MINOR=\"$$ANTS2_MINOR\"
DEFINES += ANTS2_MAJOR=\"$$ANTS2_MAJOR\"
DEFINES += DEBUG_VERBOSITY=\"$$DEBUG_VERBOSITY\"
DEFINES += TARGET_DIR=\"\\\"$${OUT_PWD}\\\"\"

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

        todir = $${OUT_PWD}/DATA
        fromdir = $${PWD}/DATA
        todir ~= s,/,\\,g
        fromdir ~= s,/,\\,g
        QMAKE_PRE_LINK = $$quote(cmd /c xcopy \"$${fromdir}\" \"$${todir}\" /y /e /i$$escape_expand(\n\t))
      }

linux-g++ {
   #todir = $${OUT_PWD}/..
   todir = $${OUT_PWD}
   fromdir = $${PWD}/EXAMPLES
   QMAKE_POST_LINK = $$quote(cp -rf \"$${fromdir}\" \"$${todir}\"$$escape_expand(\n\t))

   todir = $${OUT_PWD}
   fromdir = $${PWD}/DATA
   QMAKE_PRE_LINK = $$quote(cp -rf \"$${fromdir}\" \"$${todir}\"$$escape_expand(\n\t))
}

unix {
   #todir = $${OUT_PWD}/..
   todir = $${OUT_PWD}
   fromdir = $${PWD}/EXAMPLES
   QMAKE_POST_LINK = $$quote(cp -rf \"$${fromdir}\" \"$${todir}\"$$escape_expand(\n\t))

   todir = $${OUT_PWD}
   fromdir = $${PWD}/DATA
   QMAKE_PRE_LINK = $$quote(cp -rf \"$${fromdir}\" \"$${todir}\"$$escape_expand(\n\t))
}
#------------

FORMS += \
    gui/ageant4configdialog.ui \
    gui/MainWindowTools/alogconfigdialog.ui \
    gui/GraphWindowTools/alegenddialog.ui \
    gui/MainWindowTools/aroottextconfigurator.ui \
    gui/GraphWindowTools/aaxesdialog.ui \
    gui/GraphWindowTools/atextpavedialog.ui \
    gui/GraphWindowTools/alinemarkerfilldialog.ui \
    gui/GraphWindowTools/arootcolorselectordialog.ui \
    gui/GraphWindowTools/atemplateselectiondialog.ui \
    gui/amateriallibrarybrowser.ui \
    gui/DAWindowTools/amaterialloaderdialog.ui \
    gui/MainWindowTools/asaveparticlestofiledialog.ui

