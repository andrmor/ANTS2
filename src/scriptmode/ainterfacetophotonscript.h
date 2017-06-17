#ifndef AINTERFACETOPHOTONSCRIPT_H
#define AINTERFACETOPHOTONSCRIPT_H

#include "scriptinterfaces.h"

#include <QVector>

class AConfiguration;
class DetectorClass;
class APhotonTracer;
class TrackHolderClass;
class OneEventClass;
class ASimulationStatistics;
class MainWindow;
class TH1;

class AInterfaceToPhotonScript : public AScriptInterface
{
  Q_OBJECT
public:
    AInterfaceToPhotonScript (AConfiguration* Config, MainWindow* MW = 0); //Main window is optional!
    ~AInterfaceToPhotonScript();

public slots:
    bool TracePhotons(int copies, double x, double y, double z, double vx, double vy, double vz, int iWave = 0, double time = 0);

private:
    AConfiguration* Config;
    DetectorClass* Detector;
    MainWindow* MW = 0;

    APhotonTracer* Tracer;

    ASimulationStatistics* Stat;
    bool bBuildTracks = false;
    int MaxTracks = 1000;
    QVector<TrackHolderClass *> Tracks;
    OneEventClass* Event;

    void clearTracks();
    void addTH1(TH1 *first, const TH1 *second);
    void normalizeVector(double *arr);
};

#endif // AINTERFACETOPHOTONSCRIPT_H
