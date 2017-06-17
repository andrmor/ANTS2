#include "ainterfacetophotonscript.h"
#include "aconfiguration.h"
#include "aphotontracer.h"
#include "detectorclass.h"
#include "asandwich.h"
#include "oneeventclass.h"
#include "generalsimsettings.h"
#include "asimulationstatistics.h"
#include "aphoton.h"
#include "atrackrecords.h"
#include "TMath.h"

#include "mainwindow.h"
#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "TGeoTrack.h"
#include "TGeoManager.h"
#include "TH1.h"


#include <QDebug>

AInterfaceToPhotonScript::AInterfaceToPhotonScript(AConfiguration* Config, MainWindow* MW) :
    Config(Config), Detector(Config->GetDetector()), MW(MW)
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

bool AInterfaceToPhotonScript::TracePhotons(int copies, double x, double y, double z, double vx, double vy, double vz, int iWave, double time)
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

    simSet.fQEaccelerator = false;
    bBuildTracks = true;
    simSet.MaxNumberOfTracks = 10000;

    Event->configure(&simSet);
    Stat->initialize();
    Tracer->configure(&simSet, Event, bBuildTracks, &Tracks);
    clearTracks();

    double r[3];
    r[0]=x;
    r[1]=y;
    r[2]=z;
    double v[3];
    v[0] = vx;
    v[1] = vy;
    v[2] = vz;
    normalizeVector(v);

    APhoton* phot = new APhoton(r, v, iWave, time);
    phot->SimStat = Stat;
    for (int i=0; i<copies; i++)
    {
        Tracer->TracePhoton(phot);
    }

    if (MW)
    {
        MW->EventsDataHub->SimStat->initialize();
        MW->EventsDataHub->SimStat->AppendSimulationStatistics(Stat);
        if (simSet.fAngResolved)  addTH1(MW->EventsDataHub->SimStat->getCosAngleSpectrum(), Stat->getCosAngleSpectrum());
        if (simSet.fTimeResolved) addTH1(MW->EventsDataHub->SimStat->getTimeSpectrum(),     Stat->getTimeSpectrum());
        if (simSet.fWaveResolved) addTH1(MW->EventsDataHub->SimStat->getWaveSpectrum(),     Stat->getWaveSpectrum());
        addTH1(MW->EventsDataHub->SimStat->getTransitionSpectrum(), Stat->getTransitionSpectrum());

        Detector->GeoManager->ClearTracks();

        if (true)
          {
            int numTracks = 0;
            for (int iTr=0; iTr<Tracks.size(); iTr++)
              {
                TrackHolderClass* th = Tracks.at(iTr);
                TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
                track->SetLineColor(1);
                track->SetLineWidth(th->Width);
                for (int iNode=0; iNode<th->Nodes.size(); iNode++)
                    track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);
                if (track->GetNpoints()>1)
                {
                    numTracks++;
                    Detector->GeoManager->AddTrack(track);
                }
                else delete track;
              }
          }
    }

    return true;
}

void AInterfaceToPhotonScript::clearTracks()
{
    for(int i=0; i<Tracks.size(); i++)
        delete Tracks[i];
    Tracks.clear();
}

void AInterfaceToPhotonScript::addTH1(TH1 *first, const TH1 *second)
{
    int bins = second->GetNbinsX();
    for(int i = 0; i < bins; i++)
        first->Fill(second->GetBinCenter(i), second->GetBinContent(i));
}

void AInterfaceToPhotonScript::normalizeVector(double *arr)
{
    double Norm = 0;
    for (int i=0;i<3;i++) Norm += arr[i]*arr[i];
    Norm = TMath::Sqrt(Norm);
    if (Norm < 1e-20)
    {
        arr[0] = 1.0;
        arr[1] = 0;
        arr[2] = 0;
        qWarning() << "Error in vector normalization! Using (1,0,0)";
    }
    else
    for (int i=0; i<3; i++) arr[i] = arr[i]/Norm;
}
