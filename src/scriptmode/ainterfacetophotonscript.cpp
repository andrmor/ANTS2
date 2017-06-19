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

#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "TGeoTrack.h"
#include "TGeoManager.h"
#include "TH1.h"


#include <QDebug>

AInterfaceToPhotonScript::AInterfaceToPhotonScript(AConfiguration* Config, EventsDataClass* EventsDataHub) :
    Config(Config), EventsDataHub(EventsDataHub), Detector(Config->GetDetector())
{
    Event = new OneEventClass(Detector->PMs, Detector->RandGen, EventsDataHub->SimStat);
    Tracer = new APhotonTracer(Detector->GeoManager, Detector->RandGen, Detector->MpCollection, Detector->PMs, &Detector->Sandwich->GridRecords);
}

AInterfaceToPhotonScript::~AInterfaceToPhotonScript()
{
    delete Tracer;
    delete Event;
}

void AInterfaceToPhotonScript::ClearData()
{
    EventsDataHub->clear();    
    Detector->GeoManager->ClearTracks();
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
    simSet.fLogsStat = true;
    simSet.MaxNumberOfTracks = MaxNumberTracks;

    Event->configure(&simSet);    
    Tracer->configure(&simSet, Event, bBuildTracks, &Tracks);
    clearTrackHolder();

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
    phot->SimStat = EventsDataHub->SimStat;

    for (int i=0; i<copies; i++) Tracer->TracePhoton(phot);

    Event->HitsToSignal();
    EventsDataHub->Events.append(Event->PMsignals);

    if (bBuildTracks)
    {
        int numTracks = 0;
        for (int iTr=0; iTr<Tracks.size() && numTracks < MaxNumberTracks; iTr++)
        {
            TrackHolderClass* th = Tracks.at(iTr);
            TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
            track->SetLineColor(TrackColor);
            track->SetLineWidth(TrackWidth);
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

    qDebug() << "PhLog size:"<<EventsDataHub->SimStat->PhotonHistoryLog.size();

    return true;
}

long AInterfaceToPhotonScript::GetBulkAbsorbed() const
{
    return EventsDataHub->SimStat->Absorbed;
}

long AInterfaceToPhotonScript::GetOverrideLoss() const
{
    return EventsDataHub->SimStat->OverrideLoss;
}

long AInterfaceToPhotonScript::GetHitPM() const
{
    return EventsDataHub->SimStat->HitPM;
}

long AInterfaceToPhotonScript::GetHitDummy() const
{
    return EventsDataHub->SimStat->HitDummy;
}

long AInterfaceToPhotonScript::GetEscaped() const
{
    return EventsDataHub->SimStat->Escaped;
}

long AInterfaceToPhotonScript::GetLossOnGrid() const
{
    return EventsDataHub->SimStat->LossOnGrid;
}

long AInterfaceToPhotonScript::GetTracingSkipped() const
{
    return EventsDataHub->SimStat->TracingSkipped;
}

long AInterfaceToPhotonScript::GetMaxCyclesReached() const
{
    return EventsDataHub->SimStat->MaxCyclesReached;
}

long AInterfaceToPhotonScript::GetGeneratedOutsideGeometry() const
{
    return EventsDataHub->SimStat->GeneratedOutsideGeometry;
}

long AInterfaceToPhotonScript::GetFresnelTransmitted() const
{
    return EventsDataHub->SimStat->FresnelTransmitted;
}

long AInterfaceToPhotonScript::GetFresnelReflected() const
{
    return EventsDataHub->SimStat->FresnelReflected;
}

long AInterfaceToPhotonScript::GetRayleigh() const
{
    return EventsDataHub->SimStat->Rayleigh;
}

long AInterfaceToPhotonScript::GetReemitted() const
{
  return EventsDataHub->SimStat->Reemission;
}

QVariant AInterfaceToPhotonScript::GetHistory() const
{
  QJsonArray arr;

  const QVector< QVector <APhotonHistoryLog> > &AllPhLog = EventsDataHub->SimStat->PhotonHistoryLog;
  for (int iPh=0; iPh<AllPhLog.size(); iPh++)
    {
      const QVector <APhotonHistoryLog> &ThisPhLog = AllPhLog.at(iPh);
      QJsonArray nodeArr;
      for (int iR=0; iR<ThisPhLog.size(); iR++)
        {
          const APhotonHistoryLog &rec = ThisPhLog.at(iR);
          QJsonObject ob;

          QJsonArray pos;
          pos << rec.r[0] << rec.r[1] << rec.r[2];
          ob["position"] = pos;
          ob["time"] = rec.time;
          ob["matIndex"] = rec.matIndex;
          ob["matIndexAfter"] = rec.matIndexAfter;
          ob["nodeType"] = static_cast<int>(rec.node);

          nodeArr << ob;
        }

      arr << nodeArr;
    }

  return arr.toVariantList();
}

QString AInterfaceToPhotonScript::GetProcessName(int NodeType)
{
  return APhotonHistoryLog::GetProcessName(NodeType);
}

QString AInterfaceToPhotonScript::PrintRecord(int iPhoton, int iRecord)
{
  if (iPhoton<0 || iPhoton>=EventsDataHub->SimStat->PhotonHistoryLog.size()) return "Invalid photon index";
  if (iRecord<0 || iRecord>=EventsDataHub->SimStat->PhotonHistoryLog.at(iPhoton).size()) return "Invalid record index";

  return EventsDataHub->SimStat->PhotonHistoryLog.at(iPhoton).at(iRecord).Print();
}

void AInterfaceToPhotonScript::clearTrackHolder()
{
    for(int i=0; i<Tracks.size(); i++)
        delete Tracks[i];
    Tracks.clear();
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
