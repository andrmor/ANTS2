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

class AInterfaceToPhotonScript : public AScriptInterface
{
  Q_OBJECT
public:
    AInterfaceToPhotonScript (AConfiguration* Config);
    ~AInterfaceToPhotonScript();

public slots:
    bool TracePhotons(int copies, double x0, double y0, double z0, double v0, double v1, double v2, int iWave = 0, double t0 = 0);

private:
    AConfiguration* Config;
    DetectorClass* Detector;

    APhotonTracer* Tracer;

    ASimulationStatistics* Stat;
    bool bBuildTracks = false;
    int MaxTracks = 1000;
    QVector<TrackHolderClass *> Tracks;
    OneEventClass* Event;
};

#endif // AINTERFACETOPHOTONSCRIPT_H
