#include "ainterfacetophotonscript.h"
#include "aconfiguration.h"
#include "aphotontracer.h"
#include "detectorclass.h"
#include "asandwich.h"
#include "oneeventclass.h"
#include "generalsimsettings.h"
#include "asimulationstatistics.h"

#include <QDebug>

AInterfaceToPhotonScript::AInterfaceToPhotonScript(AConfiguration* Config) :
    Config(Config), Detector(Config->GetDetector())
{
    Stat = new ASimulationStatistics();
    Event = new OneEventClass(Detector->PMs, Detector->RandGen, Stat);
    Tracer = new APhotonTracer(Detector->GeoManager, Detector->RandGen, Detector->MpCollection, Detector->PMs, &Detector->Sandwich->GridRecords);
}

AInterfaceToPhotonScript::~AInterfaceToPhotonScript()
{
    delete Tracer;
    delete Event;
    delete Stat;
}

bool AInterfaceToPhotonScript::TracePhotons(int copies, double x0, double y0, double z0, double v0, double v1, double v2, int iWave, double t0)
{
    Tracer->UpdateGeoManager(Detector->GeoManager);

    GeneralSimSettings simSet;
    QJsonObject jsSimSet = Config->JSON["SimulationConfig"].toObject();
    bool ok = simSet.readFromJson(jsSimSet);
    if (!ok)
    {
        qWarning() << "Config does not contain simulation settings!";
        return false;
    }

    Event->configure(&simSet);
    Stat->initialize();
    Tracer->configure(&simSet, Event, bBuildTracks, &Tracks);

    return true;
}
