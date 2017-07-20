#include "ainterfacetophotonscript.h"
#include "aconfiguration.h"
#include "aphotontracer.h"
#include "detectorclass.h"
#include "asandwich.h"
#include "oneeventclass.h"
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

AInterfaceToPhotonScript::AInterfaceToPhotonScript(AConfiguration* Config, EventsDataClass* EventsDataHub) :
    Config(Config), EventsDataHub(EventsDataHub), Detector(Config->GetDetector())
{
    Event = new OneEventClass(Detector->PMs, Detector->RandGen, EventsDataHub->SimStat);
    Tracer = new APhotonTracer(Detector->GeoManager, Detector->RandGen, Detector->MpCollection, Detector->PMs, &Detector->Sandwich->GridRecords);
    ClearHistoryFilters();

    bBuildTracks = false;
    TrackColor = 7;
    TrackWidth = 1;
    MaxNumberTracks = 1000;
}

AInterfaceToPhotonScript::~AInterfaceToPhotonScript()
{
    delete Tracer;
    delete Event;
}

void AInterfaceToPhotonScript::ClearData()
{
    EventsDataHub->clear();
    clearTrackHolder();
}

void AInterfaceToPhotonScript::ClearTracks()
{
    Detector->GeoManager->ClearTracks();
}

bool AInterfaceToPhotonScript::TracePhotons(int copies, double x, double y, double z, double vx, double vy, double vz, int iWave, double time)
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

    Event->HitsToSignal();
    EventsDataHub->Events.append(Event->PMsignals);

    processTracks();
    delete phot;
    return true;
}

bool AInterfaceToPhotonScript::TracePhotonsIsotropic(int copies, double x, double y, double z, int iWave, double time)
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

   Event->HitsToSignal();
   EventsDataHub->Events.append(Event->PMsignals);

   processTracks();
   delete phot;
   return true;
}

void AInterfaceToPhotonScript::SetHistoryFilters_Processes(QVariant MustInclude, QVariant MustNotInclude)
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

void AInterfaceToPhotonScript::SetHistoryFilters_Volumes(QVariant MustInclude, QVariant MustNotInclude)
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

void AInterfaceToPhotonScript::ClearHistoryFilters()
{
    EventsDataHub->SimStat->MustInclude_Processes.clear();
    EventsDataHub->SimStat->MustInclude_Volumes.clear();
    EventsDataHub->SimStat->MustNotInclude_Processes.clear();
    EventsDataHub->SimStat->MustNotInclude_Volumes.clear();
}

void AInterfaceToPhotonScript::SetRandomGeneratorSeed(int seed)
{
    Detector->RandGen->SetSeed(seed);
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

long AInterfaceToPhotonScript::GetStoppedByMonitor() const
{
    return EventsDataHub->SimStat->KilledByMonitor;
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

int AInterfaceToPhotonScript::GetHistoryLength() const
{
    return EventsDataHub->SimStat->PhotonHistoryLog.size();
}

QVariant AInterfaceToPhotonScript::GetHistory() const
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

void AInterfaceToPhotonScript::DeleteHistoryRecord(int iPhoton)
{
    if (iPhoton<0 || iPhoton>=EventsDataHub->SimStat->PhotonHistoryLog.size()) return;
    EventsDataHub->SimStat->PhotonHistoryLog.remove(iPhoton);
}

bool AInterfaceToPhotonScript::SaveHistoryToFile(QString FileName, bool AllowAppend, int StartFrom)
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

void AInterfaceToPhotonScript::AddTrackFromHistory(int iPhoton, int TrackColor, int TrackWidth)
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

int AInterfaceToPhotonScript::GetNumberOfTracks() const
{
    return Detector->GeoManager->GetNtracks();
}

QString AInterfaceToPhotonScript::PrintAllDefinedRecordMemebers()
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

QString AInterfaceToPhotonScript::GetProcessName(int NodeType)
{
  return APhotonHistoryLog::GetProcessName(NodeType);
}

QString AInterfaceToPhotonScript::PrintRecord(int iPhoton, int iRecord)
{
  if (iPhoton<0 || iPhoton>=EventsDataHub->SimStat->PhotonHistoryLog.size()) return "Invalid photon index";
  if (iRecord<0 || iRecord>=EventsDataHub->SimStat->PhotonHistoryLog.at(iPhoton).size()) return "Invalid record index";

  return EventsDataHub->SimStat->PhotonHistoryLog.at(iPhoton).at(iRecord).Print(Detector->MpCollection);
}

QString AInterfaceToPhotonScript::PrintAllDefinedProcessTypes()
{
  return APhotonHistoryLog::PrintAllProcessTypes();
}

void AInterfaceToPhotonScript::clearTrackHolder()
{
    for(int i=0; i<Tracks.size(); i++) delete Tracks[i];
    Tracks.clear();
    Tracks.squeeze();
}

bool AInterfaceToPhotonScript::initTracer()
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
    simSet.MaxNumberOfTracks = MaxNumberTracks - AlreadyStoredTracks;

    Event->configure(&simSet);
    bool bBuildTracksThisTime = bBuildTracks && (AlreadyStoredTracks < MaxNumberTracks);
    //qDebug() << bBuildTracksThisTime<< simSet.MaxNumberOfTracks<<AlreadyStoredTracks << MaxNumberTracks;
    Tracer->configure(&simSet, Event, bBuildTracksThisTime, &Tracks);

    return true;
}

void AInterfaceToPhotonScript::processTracks()
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
