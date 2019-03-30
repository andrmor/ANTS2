#include "aphoton_si.h"
#include "aconfiguration.h"
#include "aphotontracer.h"
#include "detectorclass.h"
#include "asandwich.h"
#include "aoneevent.h"
#include "asimulationstatistics.h"
#include "aphoton.h"
#include "atrackrecords.h"
#include "TMath.h"
#include "TRandom2.h"

#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "TGeoTrack.h"
#include "TGeoManager.h"
#include "TH1.h"

#include <QDebug>
#include <QFileInfo>

APhoton_SI::APhoton_SI(AConfiguration* Config, EventsDataClass* EventsDataHub) :
    Config(Config), EventsDataHub(EventsDataHub), Detector(Config->GetDetector())
{
    Event = new AOneEvent(Detector->PMs, Detector->RandGen, EventsDataHub->SimStat);
    Tracer = new APhotonTracer(Detector->GeoManager, Detector->RandGen, Detector->MpCollection, Detector->PMs, &Detector->Sandwich->GridRecords);
    ClearHistoryFilters();

    bBuildTracks = false;
    TrackColor = 7;
    TrackWidth = 1;
    MaxNumberTracks = 1000;
}

APhoton_SI::~APhoton_SI()
{
    delete Tracer;
    delete Event;
}

void APhoton_SI::ClearData()
{
    EventsDataHub->clear();
    clearTrackHolder();
}

void APhoton_SI::ClearTracks()
{
    Detector->GeoManager->ClearTracks();
}

bool APhoton_SI::TracePhotons(int copies, double x, double y, double z, double vx, double vy, double vz, int iWave, double time, bool AddToPreviousEvent)
{
    if (!initTracer()) return false;

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

    handleEventData(AddToPreviousEvent);

    delete phot;
    return true;
}

bool APhoton_SI::TracePhotonsIsotropic(int copies, double x, double y, double z, int iWave, double time, bool AddToPreviousEvent)
{
   if (!initTracer()) return false;

   double r[3];
   r[0]=x;
   r[1]=y;
   r[2]=z;

   double v[3];  //will be defined for each photon individually
   APhoton* phot = new APhoton(r, v, iWave, time);
   phot->SimStat = EventsDataHub->SimStat;

   for (int i=0; i<copies; i++)
   {
       //Sphere function of Root:
       double a=0, b=0, r2=1.0;
       while (r2 > 0.25)
         {
             a  = Detector->RandGen->Rndm() - 0.5;
             b  = Detector->RandGen->Rndm() - 0.5;
             r2 =  a*a + b*b;
         }
       phot->v[2] = ( -1.0 + 8.0 * r2 );
       double scale = 8.0 * TMath::Sqrt(0.25 - r2);
       phot->v[0] = a*scale;
       phot->v[1] = b*scale;
       Tracer->TracePhoton(phot);
   }

   handleEventData(AddToPreviousEvent);

   delete phot;
   return true;
}

bool APhoton_SI::TracePhotonsS2Isotropic(int copies, double x, double y, double zStart, double zStop, int iWave, double time, double dZmm_dTns, bool AddToPreviousEvent)
{
    if (!initTracer()) return false;

    double r[3];
    r[0]=x;
    r[1]=y;
    double zSpan = zStop-zStart;
    if (zSpan < 0) return false;
    double invVelo = 1.0 / dZmm_dTns;

    double v[3];  //will be defined for each photon individually
    APhoton* phot = new APhoton(r, v, iWave, time);
    phot->SimStat = EventsDataHub->SimStat;

    for (int i=0; i<copies; i++)
    {
        //Generating direction ('Sphere' function of Root)
        double a=0, b=0, r2=1.0;
        while (r2 > 0.25)
          {
              a  = Detector->RandGen->Rndm() - 0.5;
              b  = Detector->RandGen->Rndm() - 0.5;
              r2 =  a*a + b*b;
          }
        phot->v[2] = ( -1.0 + 8.0 * r2 );
        double scale = 8.0 * TMath::Sqrt(0.25 - r2);
        phot->v[0] = a*scale;
        phot->v[1] = b*scale;

        //generating z
        double dz = zSpan * Detector->RandGen->Rndm();
        phot->r[2] = zStart + dz;
        phot->time = time + dz * invVelo;

        //tracing photons
        Tracer->TracePhoton(phot);
    }

    handleEventData(AddToPreviousEvent);

    delete phot;
    return true;
}

void APhoton_SI::handleEventData(bool AddToPreviousEvent)
{
    Event->HitsToSignal();

    const bool bTimed = !Event->TimedPMsignals.isEmpty();
    const bool bAlreadyHaveEvent = !EventsDataHub->Events.isEmpty();
    bool bDataContainersMatch = false;
    if (bAlreadyHaveEvent)
    {
        if ( EventsDataHub->Events.last().size() == Event->PMsignals.size() )
        {
            if (bTimed)
            {
                if (EventsDataHub->TimedEvents.last().size() == Event->TimedPMsignals.size()) //matching number of time bins
                {
                    if (EventsDataHub->TimedEvents.last().last().size() == Event->TimedPMsignals.last().size()) //matching number of PMs
                       bDataContainersMatch = true;
                }
            }
            else bDataContainersMatch = true;
        }
    }
    //qDebug() << "timed, alreadyhave, match:"<<bTimed<<bAlreadyHaveEvent<<bDataContainersMatch;

    if ( AddToPreviousEvent && bDataContainersMatch )
    {
        for (int i=0; i<Event->PMsignals.size(); i++)
            EventsDataHub->Events.last()[i] += Event->PMsignals.at(i);
        if (bTimed)
            for (int i=0; i<Event->TimedPMsignals.size(); i++)
                for (int ii=0; ii<Event->TimedPMsignals.at(i).size(); ii++)
                     EventsDataHub->TimedEvents.last()[i][ii] += Event->TimedPMsignals[i][ii];
    }
    else
    {
        EventsDataHub->Events.append(Event->PMsignals);
        if (bTimed) EventsDataHub->TimedEvents.append(Event->TimedPMsignals);
    }

    processTracks();
    EventsDataHub->LastSimSet = simSet;
}

void APhoton_SI::SetHistoryFilters_Processes(QVariant MustInclude, QVariant MustNotInclude)
{
  QVariantList vMI = MustInclude.toList();
  QJsonArray arMI = QJsonArray::fromVariantList(vMI);
  QVariantList vMNI = MustNotInclude.toList();
  QJsonArray arMNI = QJsonArray::fromVariantList(vMNI);

  EventsDataHub->SimStat->MustInclude_Processes.clear();
  EventsDataHub->SimStat->MustNotInclude_Processes.clear();
  for (int i=0; i<arMI.size(); i++)
    {
      if (!arMI[i].isDouble()) continue;
      EventsDataHub->SimStat->MustInclude_Processes << arMI[i].toInt();
    }
  for (int i=0; i<arMNI.size(); i++)
    {
      if (!arMNI[i].isDouble()) continue;
      EventsDataHub->SimStat->MustNotInclude_Processes << arMNI[i].toInt();
    }
}

void APhoton_SI::SetHistoryFilters_Volumes(QVariant MustInclude, QVariant MustNotInclude)
{
  QVariantList vMI = MustInclude.toList();
  QJsonArray arMI = QJsonArray::fromVariantList(vMI);
  QVariantList vMNI = MustNotInclude.toList();
  QJsonArray arMNI = QJsonArray::fromVariantList(vMNI);

  EventsDataHub->SimStat->MustNotInclude_Volumes.clear();
  EventsDataHub->SimStat->MustInclude_Volumes.clear();
  for (int i=0; i<arMI.size(); i++)
    {
      if (!arMI[i].isString()) continue;
      EventsDataHub->SimStat->MustInclude_Volumes << arMI[i].toString();
    }
  for (int i=0; i<arMNI.size(); i++)
    {
      if (!arMNI[i].isString()) continue;
      EventsDataHub->SimStat->MustNotInclude_Volumes << arMNI[i].toString();
    }
}

void APhoton_SI::ClearHistoryFilters()
{
    EventsDataHub->SimStat->MustInclude_Processes.clear();
    EventsDataHub->SimStat->MustInclude_Volumes.clear();
    EventsDataHub->SimStat->MustNotInclude_Processes.clear();
    EventsDataHub->SimStat->MustNotInclude_Volumes.clear();
}

void APhoton_SI::SetRandomGeneratorSeed(int seed)
{
    Detector->RandGen->SetSeed(seed);
}

long APhoton_SI::GetBulkAbsorbed() const
{
    return EventsDataHub->SimStat->Absorbed;
}

long APhoton_SI::GetOverrideLoss() const
{
    return EventsDataHub->SimStat->OverrideLoss;
}

long APhoton_SI::GetHitPM() const
{
    return EventsDataHub->SimStat->HitPM;
}

long APhoton_SI::GetHitDummy() const
{
    return EventsDataHub->SimStat->HitDummy;
}

long APhoton_SI::GetEscaped() const
{
    return EventsDataHub->SimStat->Escaped;
}

long APhoton_SI::GetLossOnGrid() const
{
    return EventsDataHub->SimStat->LossOnGrid;
}

long APhoton_SI::GetTracingSkipped() const
{
    return EventsDataHub->SimStat->TracingSkipped;
}

long APhoton_SI::GetMaxCyclesReached() const
{
    return EventsDataHub->SimStat->MaxCyclesReached;
}

long APhoton_SI::GetGeneratedOutsideGeometry() const
{
    return EventsDataHub->SimStat->GeneratedOutsideGeometry;
}

long APhoton_SI::GetStoppedByMonitor() const
{
    return EventsDataHub->SimStat->KilledByMonitor;
}

long APhoton_SI::GetFresnelTransmitted() const
{
    return EventsDataHub->SimStat->FresnelTransmitted;
}

long APhoton_SI::GetFresnelReflected() const
{
    return EventsDataHub->SimStat->FresnelReflected;
}

long APhoton_SI::GetRayleigh() const
{
    return EventsDataHub->SimStat->Rayleigh;
}

long APhoton_SI::GetReemitted() const
{
    return EventsDataHub->SimStat->Reemission;
}

int APhoton_SI::GetHistoryLength() const
{
    return EventsDataHub->SimStat->PhotonHistoryLog.size();
}

QVariant APhoton_SI::GetHistory() const
{
  //qDebug() << "   get history triggered"<< EventsDataHub->SimStat->PhotonHistoryLog.capacity();
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
          ob["iMat"] = rec.matIndex;
          ob["iMatNext"] = rec.matIndexAfter;
          ob["process"] = static_cast<int>(rec.process);
          ob["volumeName"] = rec.volumeName;
          ob["number"] = rec.number;
          ob["wave"] = rec.iWave;

          nodeArr << ob;
        }

      arr << nodeArr;
      //if (iPh%100 == 0)qDebug() << iPh << "of" << AllPhLog.size();
    }
  //qDebug() << "   sending history to script...";
  return arr.toVariantList();
}

void APhoton_SI::DeleteHistoryRecord(int iPhoton)
{
    if (iPhoton<0 || iPhoton>=EventsDataHub->SimStat->PhotonHistoryLog.size()) return;
    EventsDataHub->SimStat->PhotonHistoryLog.remove(iPhoton);
}

bool APhoton_SI::SaveHistoryToFile(QString FileName, bool AllowAppend, int StartFrom)
{
    if (!AllowAppend && QFileInfo(FileName).exists())
      {
        abort("File already exists and append was not allowed!");
        return false;
      }

    QFile file(FileName);
    if ( !file.open(QIODevice::Append ) )
    {
        abort("Cannot open file: " + FileName);
        return false;
    }

    QTextStream s(&file);
    const QVector< QVector <APhotonHistoryLog> > &AllPhLog = EventsDataHub->SimStat->PhotonHistoryLog;
    for (int iPh=StartFrom; iPh<AllPhLog.size(); iPh++)
      {
        const QVector <APhotonHistoryLog> &ThisPhLog = AllPhLog.at(iPh);
        for (int iR=0; iR<ThisPhLog.size(); iR++)
          {
            const APhotonHistoryLog &rec = ThisPhLog.at(iR);

            s << iR << "   "
              << APhotonHistoryLog::GetProcessName(rec.process) << "   "
              << rec.r[0] <<" "<< rec.r[1] <<" "<< rec.r[2] << " " <<rec.time << "  "
              << rec.volumeName << "  "
              << rec.number << "  "
              << rec.matIndex << " " << rec.matIndexAfter << "   "
              << rec.iWave
              << "\r\n";
          }
        s << "\r\n";
      }
    file.close();

    return true;
}

void APhoton_SI::AddTrackFromHistory(int iPhoton, int TrackColor, int TrackWidth)
{
    if (iPhoton<0 || iPhoton>=EventsDataHub->SimStat->PhotonHistoryLog.size()) return;

    const QVector <APhotonHistoryLog> &ThisPhLog = EventsDataHub->SimStat->PhotonHistoryLog.at(iPhoton);
    if (ThisPhLog.size()<2) return;

    TGeoTrack* track = new TGeoTrack(1, 1);
    track->SetLineColor(TrackColor);
    track->SetLineWidth(TrackWidth);
    for (int iR=0; iR<ThisPhLog.size(); iR++)
    {
        const APhotonHistoryLog &rec = ThisPhLog.at(iR);
        track->AddPoint(rec.r[0], rec.r[1], rec.r[2], rec.time);
    }
    Detector->GeoManager->AddTrack(track);
}

int APhoton_SI::GetNumberOfTracks() const
{
    return Detector->GeoManager->GetNtracks();
}

QString APhoton_SI::PrintAllDefinedRecordMemebers()
{
  QString s = "<br>Defined record fields:<br>";
  s += "process -> process type<br>";
  s += "position -> array of x, y and z<br>";
  s += "volumeName -> name of the current geometry volume<br>";
  s += "time -> time in ns<br>";
  s += "iMat -> material index of this volume (-1 if undefined)<br>";
  s += "iMatNext -> material index after interface (-1 if undefined)<br>";
  s += "number -> index of PM hit (-1 if undefined)<br>";
  s += "wave -> wavelength index (-1 if undefined)<br>";
  return s;
}

QString APhoton_SI::GetProcessName(int NodeType)
{
  return APhotonHistoryLog::GetProcessName(NodeType);
}

QString APhoton_SI::PrintRecord(int iPhoton, int iRecord)
{
  if (iPhoton<0 || iPhoton>=EventsDataHub->SimStat->PhotonHistoryLog.size()) return "Invalid photon index";
  if (iRecord<0 || iRecord>=EventsDataHub->SimStat->PhotonHistoryLog.at(iPhoton).size()) return "Invalid record index";

  return EventsDataHub->SimStat->PhotonHistoryLog.at(iPhoton).at(iRecord).Print(Detector->MpCollection);
}

QString APhoton_SI::PrintAllDefinedProcessTypes()
{
  return APhotonHistoryLog::PrintAllProcessTypes();
}

void APhoton_SI::clearTrackHolder()
{
    for (size_t i=0; i<Tracks.size(); i++) delete Tracks[i];
    Tracks.clear();
    Tracks.shrink_to_fit();
}

bool APhoton_SI::initTracer()
{
    Tracer->UpdateGeoManager(Detector->GeoManager);

    QJsonObject jsSimSet = Config->JSON["SimulationConfig"].toObject();
    bool ok = simSet.readFromJson(jsSimSet);
    if (!ok)
    {
        qWarning() << "Config does not contain simulation settings!";
        return false;
    }

    int AlreadyStoredTracks = Detector->GeoManager->GetNtracks();

    simSet.bDoPhotonHistoryLog = true;
    simSet.fQEaccelerator = false;    
    simSet.fLogsStat = true;
    simSet.TrackBuildOptions.MaxParticleTracks = MaxNumberTracks - AlreadyStoredTracks;

    Event->configure(&simSet);
    bool bBuildTracksThisTime = bBuildTracks && (AlreadyStoredTracks < MaxNumberTracks);
    //qDebug() << bBuildTracksThisTime<< simSet.MaxNumberOfTracks<<AlreadyStoredTracks << MaxNumberTracks;
    Tracer->configure(&simSet, Event, bBuildTracksThisTime, &Tracks);

    return true;
}

void APhoton_SI::processTracks()
{
    for (int iTr=0; iTr<Tracks.size(); iTr++)
    {
        TrackHolderClass* th = Tracks.at(iTr);
        TGeoTrack* geoTrack = new TGeoTrack(1, th->UserIndex);
        geoTrack->SetLineColor(TrackColor);
        geoTrack->SetLineWidth(TrackWidth);
        for (int iNode=0; iNode<th->Nodes.size(); iNode++)
            geoTrack->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);
        if (geoTrack->GetNpoints()>1)
        {
            Detector->GeoManager->AddTrack(geoTrack);
            if (Detector->GeoManager->GetNtracks() >= MaxNumberTracks) break;
        }
        else delete geoTrack;
    }
    clearTrackHolder();
}

void APhoton_SI::normalizeVector(double *arr)
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
