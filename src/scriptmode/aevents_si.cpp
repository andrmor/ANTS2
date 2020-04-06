#include "aevents_si.h"
#include "aconfiguration.h"
#include "eventsdataclass.h"
#include "apositionenergyrecords.h"
#include "detectorclass.h"
#include "apmhub.h"
#include "sensorlrfs.h"

#include "TGeoNode.h"
#include "TGeoManager.h"

AEvents_SI::AEvents_SI(AConfiguration *Config, EventsDataClass* EventsDataHub)
  : Config(Config), EventsDataHub(EventsDataHub)
{
  Description = "Access to PM signals and reconstruction data";

  H["GetNumPMs"] = "Number of sensors in the available events dataset. If the dataset is empty, 0 is returned.";
  H["GetNumEvents"] = "Number of available events.";
  H["GetPMsignal"] = "Get signal for the event number ievent and PM number ipm.";
  H["SetPMsignal"] = "Set signal value for the event number ievent and PM number ipm.";

  H["GetRho2"] = "Square of the distance between the reconstructed position of the ievent event and the center of the iPM photosensor";
  H["GetRho"] = "Distance between the reconstructed position of the ievent event and the center of the iPM photosensor";

  H["SetReconstructed"] = "For event number ievent set reconstructed x,y,z and energy.\n"
                          "The function automatically sets status ReconstructionOK and GoodEvent to true for this event.\n"
                          "After all events are reconstructed, SetReconstructionReady() has to be called!";

  H["GetStatistics"] = "Returns (if available) an array with GoodEvents, Average_Chi2, Average_XY_deviation";

  H["GetTruePoints"] = "Return array of true (scan) points for the event. Array elements are [x, y, z, energy]";
  H["GetTrueNumberPoints"] = "Return number of true (scan) points for the given event";


  DepRem["GetTruePoints"] = "Use GetTruePointsXYZE() or GetTruePointsXYZEiMat instead";
}

double AEvents_SI::GetPMsignal(int ievent, int ipm)
{
  int numEvents = GetNumEvents();
  int numPMs = GetNumPMs();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return 0;
    }
  if (ipm<0 || ipm>numPMs-1)
    {
      abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
      return 0;
    }

  return EventsDataHub->Events.at(ievent).at(ipm);
}

QVariant AEvents_SI::GetPMsignals(int ievent)
{
  int numEvents = GetNumEvents();
  if (ievent<0 || ievent>=numEvents)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return 0;
    }

  const QVector< float >& sigs = EventsDataHub->Events.at(ievent);
  QVariantList l;
  for (const float & f : sigs) l << QVariant(f);
  return l;
}

const QVariantList AEvents_SI::GetPMsignals() const
{
    QVariantList vl;
    vl.reserve(EventsDataHub->Events.size());

    for (const QVector<float> & event : EventsDataHub->Events)
    {
        QVariantList el;
        for (const float & sig : event) el << sig;
        vl.push_back(el);
    }
    return vl;
}

void AEvents_SI::SetPMsignal(int ievent, int ipm, double value)
{
    int numEvents = GetNumEvents();
    int numPMs = GetNumPMs();
    if (ievent<0 || ievent>numEvents-1)
      {
        abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
        return;
      }
    if (ipm<0 || ipm>numPMs-1)
      {
        abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
        return;
      }

    EventsDataHub->Events[ievent][ipm] = value;
}

double AEvents_SI::GetPMsignalTimed(int ievent, int ipm, int iTimeBin)
{
    int numEvents = countTimedEvents();
    if (ievent<0 || ievent >= numEvents)
      {
        abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
        return 0;
      }

    int numTimeBins = countTimeBins();
    if (iTimeBin<0 || iTimeBin >= numTimeBins)
      {
        abort("Wrong time bin "+QString::number(ievent)+" Time bins available: "+QString::number(numTimeBins));
        return 0;
      }

    int numPMs = EventsDataHub->TimedEvents.at(ievent).at(iTimeBin).size();
    if (ipm<0 || ipm>=numPMs)
      {
        abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
        return 0;
      }

    return EventsDataHub->TimedEvents.at(ievent).at(iTimeBin).at(ipm);
}

QVariant AEvents_SI::GetPMsignalVsTime(int ievent, int ipm)
{
    int numEvents = countTimedEvents();
    if (ievent<0 || ievent >= numEvents)
      {
        abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
        return 0;
      }

    int numTimeBins = countTimeBins();
    if (numTimeBins == 0) return QVariantList();

    int numPMs = EventsDataHub->TimedEvents.at(ievent).first().size();
    if (ipm<0 || ipm>=numPMs)
      {
        abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
        return 0;
      }

    QVariantList aa;
    for (int i=0; i<numTimeBins; i++)
        aa << EventsDataHub->TimedEvents.at(ievent).at(i).at(ipm);
    return aa;
}

int AEvents_SI::GetNumPMs()
{
  if (EventsDataHub->Events.isEmpty()) return 0;
  return EventsDataHub->Events.first().size();
}

int AEvents_SI::countPMs()
{
    if (EventsDataHub->Events.isEmpty()) return 0;
    return EventsDataHub->Events.first().size();
}

int AEvents_SI::GetNumEvents()
{
    return EventsDataHub->Events.size();
}

int AEvents_SI::countEvents()
{
    return EventsDataHub->Events.size();
}

int AEvents_SI::countTimedEvents()
{
    return EventsDataHub->TimedEvents.size();
}

int AEvents_SI::countTimeBins()
{
    if (EventsDataHub->TimedEvents.isEmpty()) return 0;
    return EventsDataHub->TimedEvents.first().size();
}

bool AEvents_SI::checkEventNumber(int ievent)
{
  int numEvents = EventsDataHub->Events.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  return true;
}

bool AEvents_SI::checkEventNumber(int igroup, int ievent, int ipoint)
{
    int numGroups = EventsDataHub->ReconstructionData.size();
    if (igroup<0 || igroup>numGroups-1)
      {
        abort("Wrong group number "+QString::number(igroup)+" Groups available: "+QString::number(numGroups));
        return false;
      }
    //if (!EventsDataHub->isReconstructionReady())
    //  {
    //    abort("Reconstruction was not yet performed");
    //    return false;
    //  }
    int numEvents = EventsDataHub->ReconstructionData.at(igroup).size();
    if (ievent<0 || ievent>numEvents-1)
      {
        abort("Wrong event number "+QString::number(ievent)+" Reconstructed events available: "+QString::number(numEvents));
        return false;
      }
    int numPoints = EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points.size();
    if (ipoint<0 || ipoint>numPoints-1)
      {
        abort("Wrong point "+QString::number(ipoint)+" Points available: "+QString::number(numPoints));
        return false;
      }
    return true;
}

bool AEvents_SI::checkPM(int ipm)
{
  if (ipm<0 || ipm>Config->GetDetector()->PMs->count()-1)
      {
        abort("Invalid PM index: "+QString::number(ipm));
        return false;
      }
  return true;
}

bool AEvents_SI::checkSetReconstructionDataRequest(int ievent)
{
  int numEvents = EventsDataHub->Events.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  return true;
}

bool AEvents_SI::checkTrueDataRequest(int ievent, int iPoint)
{
  if (EventsDataHub->isScanEmpty())
    {
      abort("There are no scan/true data!");
      return false;
    }
  int numEvents = EventsDataHub->Scan.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  if (iPoint == -1) return true;
  if (iPoint<0 || iPoint >= EventsDataHub->Scan.at(ievent)->Points.size())
  {
      abort( QString("Bad scan point number: event #%1 has %2 point(s)").arg(ievent).arg( EventsDataHub->Scan.at(ievent)->Points.size() ));
      return false;
  }
  return true;
}

double AEvents_SI::GetReconstructedX(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0];
}

double AEvents_SI::GetReconstructedX(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[0];
}

double AEvents_SI::GetReconstructedY(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1];
}

double AEvents_SI::GetReconstructedY(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[1];
}

double AEvents_SI::GetReconstructedZ(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[2];
}

QVariantList AEvents_SI::GetReconstructedXYZ(int ievent)
{
    QVariantList vl;
    if (checkEventNumber(ievent))
        for (int i=0; i<3; i++)
            vl.append(EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[i]);

    return vl;
}

QVariantList AEvents_SI::GetReconstructedXYZE(int ievent)
{
    QVariantList vl;
    if (checkEventNumber(ievent))
    {
        for (int i=0; i<3; i++)
            vl.append(EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[i]);
        vl.append(EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].energy);
    }

    return vl;
}

double AEvents_SI::GetReconstructedZ(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[2];
}

QVariantList AEvents_SI::GetReconstructedPoints(int ievent)
{
    QVariantList list;

    if (!checkEventNumber(0, ievent, 0)) return list;

    const APositionEnergyBuffer& p = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points;
    for (int i=0; i<p.size(); i++)
    {
        QVariantList el;
        el << p.at(i).r[0];
        el << p.at(i).r[1];
        el << p.at(i).r[2];
        el << p.at(i).energy;
        list.push_back(el);
    }
    return list;
}

QVariantList AEvents_SI::GetReconstructedPoints(int igroup, int ievent)
{
    QVariantList list;

    if (!checkEventNumber(igroup, ievent, 0)) return list;

    const APositionEnergyBuffer& p = EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points;
    for (int i=0; i<p.size(); i++)
    {
        QVariantList el;
        el << p.at(i).r[0];
        el << p.at(i).r[1];
        el << p.at(i).r[2];
        el << p.at(i).energy;
        list.push_back(el);
    }
    return list;
}

double AEvents_SI::GetRho(int ievent, int iPM)
{
    if (!checkPM(iPM)) return 0;
    if (!checkEventNumber(ievent)) return 0; //anyway aborted
    return sqrt( GetRho2(ievent, iPM) );
}

double AEvents_SI::GetRho(int igroup, int ievent, int ipoint, int iPM)
{
  if (!checkPM(iPM)) return 0;
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[2];
}

double AEvents_SI::GetRho2(int ievent, int iPM)
{
  if (!checkPM(iPM)) return 0;
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  double dx2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0] - Config->GetDetector()->PMs->X(iPM);
  dx2 *= dx2;
  double dy2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1] - Config->GetDetector()->PMs->Y(iPM);
  dy2 *= dy2;
  return dx2+dy2;
}

double AEvents_SI::GetRho2(int igroup, int ievent, int ipoint, int iPM)
{
    if (!checkPM(iPM)) return 0;
    if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
    double dx2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0] - Config->GetDetector()->PMs->X(iPM);
    dx2 *= dx2;
    double dy2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1] - Config->GetDetector()->PMs->Y(iPM);
    dy2 *= dy2;
    return dx2+dy2;
}

double AEvents_SI::GetReconstructedEnergy(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].energy;
}

double AEvents_SI::GetReconstructedEnergy(int igroup, int ievent, int ipoint)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return 0;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].energy;
}

bool AEvents_SI::IsReconstructedGoodEvent(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->GoodEvent;
}

bool AEvents_SI::IsReconstructedGoodEvent(int igroup, int ievent)
{
    if (!checkEventNumber(igroup, ievent, 0)) return 0;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->GoodEvent;
}

bool AEvents_SI::IsReconstructed_ScriptFilterPassed(int igroup, int ievent)
{
  if (!checkEventNumber(igroup, ievent, 0)) return 0;
  return !EventsDataHub->ReconstructionData.at(igroup).at(ievent)->fScriptFiltered;
}

int AEvents_SI::countReconstructedGroups()
{
    return EventsDataHub->ReconstructionData.size();
}

int AEvents_SI::countReconstructedEvents(int igroup)
{
    if (igroup<0 || igroup>EventsDataHub->ReconstructionData.size()-1) return -1;
    return EventsDataHub->ReconstructionData.at(igroup).size();
}

int AEvents_SI::countReconstructedPoints(int igroup, int ievent)
{
    if (igroup<0 || igroup>EventsDataHub->ReconstructionData.size()-1) return -1;
    if (ievent<0 || ievent>EventsDataHub->ReconstructionData.at(igroup).size()-1) return -1;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points.size();
}

double AEvents_SI::GetTrueX(int ievent, int iPoint)
{
  if (!checkTrueDataRequest(ievent, iPoint)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[iPoint].r[0];
}

double AEvents_SI::GetTrueY(int ievent, int iPoint)
{
  if (!checkTrueDataRequest(ievent, iPoint)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[iPoint].r[1];
}

double AEvents_SI::GetTrueZ(int ievent, int iPoint)
{
  if (!checkTrueDataRequest(ievent, iPoint)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[iPoint].r[2];
}

double AEvents_SI::GetTrueEnergy(int ievent, int iPoint)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[iPoint].energy;
}

const QVariant AEvents_SI::GetTruePoints(int ievent)
{
  if (!checkTrueDataRequest(ievent, -1)) return 0; //anyway aborted

  QVariantList list;
  const APositionEnergyBuffer& p = EventsDataHub->Scan.at(ievent)->Points;
  for (int i=0; i<p.size(); i++)
  {
      QVariantList el;
      el << p.at(i).r[0];
      el << p.at(i).r[1];
      el << p.at(i).r[2];
      el << p.at(i).energy;
      list.push_back(el);
  }
  return list;
}

const QVariant AEvents_SI::GetTruePointsXYZE(int ievent)
{
    if (!checkTrueDataRequest(ievent, -1)) return 0; //anyway aborted

    QVariantList list;
    const APositionEnergyBuffer& p = EventsDataHub->Scan.at(ievent)->Points;
    for (int i=0; i<p.size(); i++)
    {
        QVariantList el;
        el << p.at(i).r[0];
        el << p.at(i).r[1];
        el << p.at(i).r[2];
        el << p.at(i).energy;
        list.push_back(el);
    }
    return list;
}

const QVariant AEvents_SI::GetTruePointsXYZEiMat(int ievent)
{
    if (!checkTrueDataRequest(ievent, -1)) return 0; //anyway aborted

    QVariantList list;
    const APositionEnergyBuffer& p = EventsDataHub->Scan.at(ievent)->Points;
    for (int i=0; i<p.size(); i++)
    {
        int iMat = -1;
        TGeoManager* GeoManager = Config->GetDetector()->GeoManager;
        TGeoNode* node = GeoManager->FindNode(p.at(i).r[0], p.at(i).r[1], p.at(i).r[2]);
        if (node)
            iMat = node->GetVolume()->GetMaterial()->GetIndex();

        QVariantList el;
        el << p.at(i).r[0];
        el << p.at(i).r[1];
        el << p.at(i).r[2];
        el << p.at(i).energy;
        el << iMat;
        list.push_back(el);
    }
    return list;
}

bool AEvents_SI::IsTrueGoodEvent(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->GoodEvent;
}

int AEvents_SI::GetTrueNumberPoints(int ievent)
{
  if (!checkTrueDataRequest(ievent, -1)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points.size();
}

void AEvents_SI::SetScanX(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[0] = value;
}

void AEvents_SI::SetScanY(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[1] = value;
}

void AEvents_SI::SetScanZ(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[2] = value;
}

void AEvents_SI::SetScanEnergy(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].energy = value;
}

bool pairMore (const std::pair<int, float>& p1, const std::pair<int, float>& p2)
{
    return (p1.second > p2.second);
}

QVariant AEvents_SI::GetPMsSortedBySignal(int ievent)
{
    if (!checkEventNumber(ievent)) return 0; //aborted anyway

    const int numPMs = EventsDataHub->Events.at(ievent).size();
    std::vector< std::pair<int, float> > ar;
    ar.reserve(numPMs);

    for (int i=0; i<numPMs; i++)
        ar.push_back( std::pair<int, float>(i, EventsDataHub->Events.at(ievent).at(i)) );

    std::sort(ar.begin(), ar.end(), pairMore);

    QVariantList aa;
    for (const std::pair<int, float>& p : ar )
        aa << p.first;
    return aa;
}

int AEvents_SI::GetPMwithMaxSignal(int ievent)
{
    if (!checkEventNumber(ievent)) return 0; //aborted anyway

    float MaxSig = EventsDataHub->Events.at(ievent).at(0);
    int iMaxSig = 0;
    for (int i=0; i<EventsDataHub->Events.at(ievent).size(); i++)
    {
        float sig = EventsDataHub->Events.at(ievent).at(i);
        if (sig > MaxSig)
        {
            MaxSig = sig;
            iMaxSig = i;
        }
    }
    return iMaxSig;
}

void AEvents_SI::SetReconstructed(int ievent, double x, double y, double z, double e)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = true;
}

void AEvents_SI::SetReconstructed(int ievent, double x, double y, double z, double e, double chi2)
{
    if (!checkSetReconstructionDataRequest(ievent)) return;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
    EventsDataHub->ReconstructionData[0][ievent]->chi2 = chi2;
    EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
    EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = true;
}

void AEvents_SI::SetReconstructedX(int ievent, double x)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
}

void AEvents_SI::SetReconstructedY(int ievent, double y)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
}

void AEvents_SI::SetReconstructedZ(int ievent, double z)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
}

void AEvents_SI::SetReconstructedEnergy(int ievent, double e)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
}

void AEvents_SI::SetReconstructed_ScriptFilterPass(int ievent, bool flag)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->fScriptFiltered = !flag;
}

void AEvents_SI::SetReconstructedGoodEvent(int ievent, bool good)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = good;
}

void AEvents_SI::SetReconstructedAllEventsGood(bool flag)
{
    for (int i=0; i<EventsDataHub->ReconstructionData.at(0).size(); i++)
        EventsDataHub->ReconstructionData[0][i]->GoodEvent = flag;
}

void AEvents_SI::SetReconstructionOK(int ievent, bool OK)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = OK;
  if (!OK) EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = false;
}

void AEvents_SI::SetReconstructed(int igroup, int ievent, int ipoint, double x, double y, double z, double e)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
  EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = true;
}

void AEvents_SI::SetReconstructedFast(int igroup, int ievent, int ipoint, double x, double y, double z, double e)
{
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
    EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = true;
    EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = true;
}

void AEvents_SI::AddReconstructedPoint(int igroup, int ievent, double x, double y, double z, double e)
{
  if (!checkEventNumber(igroup, ievent, 0)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points.AddPoint(x, y, z, e);
}

void AEvents_SI::SetReconstructedX(int igroup, int ievent, int ipoint, double x)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
}

void AEvents_SI::SetReconstructedY(int igroup, int ievent, int ipoint, double y)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
}

void AEvents_SI::SetReconstructedZ(int igroup, int ievent, int ipoint, double z)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
}

void AEvents_SI::SetReconstructedEnergy(int igroup, int ievent, int ipoint, double e)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
}

void AEvents_SI::SetReconstructedGoodEvent(int igroup, int ievent, int ipoint, bool good)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = good;
}

void AEvents_SI::SetReconstructed_ScriptFilterPass(int igroup, int ievent, bool flag)
{
  if (!checkEventNumber(igroup, ievent, 0)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->fScriptFiltered = !flag;
}

void AEvents_SI::SetReconstructionOK(int igroup, int ievent, int ipoint, bool OK)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = OK;
}

void AEvents_SI::SetReconstructionReady()
{
  if (!bGuiThread)
  {
      abort("Only GUI thread can do SetReconstructionReady()");
      return;
  }

  EventsDataHub->fReconstructionDataReady = true;
  emit EventsDataHub->requestEventsGuiUpdate();
}

void AEvents_SI::ResetReconstructionData(int numGroups)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do ResetReconstructionData()");
        return;
    }

    for (int ig=0; ig<numGroups; ig++)
      EventsDataHub->resetReconstructionData(numGroups);
}

void AEvents_SI::LoadEventsTree(QString fileName, bool Append, int MaxNumEvents)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do LoadEventsTree()");
        return;
    }

  if (!Append) EventsDataHub->clear();
  EventsDataHub->loadSimulatedEventsFromTree(fileName, *Config->GetDetector()->PMs, MaxNumEvents);
  //EventsDataHub->clearReconstruction();
  EventsDataHub->createDefaultReconstructionData();
  //RManager->filterEvents(Config->JSON);
  EventsDataHub->squeeze();
  emit RequestEventsGuiUpdate();
}

void AEvents_SI::LoadEventsAscii(QString fileName, bool Append)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do LoadEventsAscii()");
        return;
    }

  if (!Append) EventsDataHub->clear();
  EventsDataHub->loadEventsFromTxtFile(fileName, Config->JSON, Config->GetDetector()->PMs);
  //EventsDataHub->clearReconstruction();
  EventsDataHub->createDefaultReconstructionData();
  //RManager->filterEvents(Config->JSON);
  EventsDataHub->squeeze();
  emit RequestEventsGuiUpdate();
}

void AEvents_SI::ClearEvents()
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can clear events!");
        return;
    }

    EventsDataHub->clear(); //gui update is triggered inside
}

void AEvents_SI::PurgeBad()
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can purge events!");
        return;
    }
    EventsDataHub->PurgeFilteredEvents();
}

void AEvents_SI::Purge(int LeaveOnePer)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can purge events!");
        return;
    }
    EventsDataHub->Purge(LeaveOnePer);
}

QVariant AEvents_SI::GetStatistics(int igroup)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do GetStatistics()");
        return 0;
    }

  int GoodEvents;
  double AvChi2, AvDeviation;
  EventsDataHub->prepareStatisticsForEvents(Config->GetDetector()->LRFs->isAllLRFsDefined(), GoodEvents, AvChi2, AvDeviation, igroup);

  QVariantList l;
  l << GoodEvents << AvChi2 << AvDeviation;
  return l;
}
