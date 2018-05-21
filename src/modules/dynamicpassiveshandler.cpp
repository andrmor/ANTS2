#include "dynamicpassiveshandler.h"
#include "apmhub.h"
#include "eventsdataclass.h"
#include "reconstructionsettings.h"
#include "apositionenergyrecords.h"
#include "ajsontools.h"
#include "apmgroupsmanager.h"

#include <QDebug>

DynamicPassivesHandler::DynamicPassivesHandler(APmHub* Pms, APmGroupsManager* PMgroups, EventsDataClass *eventsDataHub)
 : PMs(Pms), PMgroups(PMgroups), EventsDataHub(eventsDataHub) { }

void DynamicPassivesHandler::init(ReconstructionSettings *RecSet, int ThisPmGroup)
{
  numPMs = PMs->count();
  this->ThisPmGroup = ThisPmGroup;
  StatePMs.resize(numPMs);
  for (int ipm=0; ipm<numPMs; ipm++)
    StatePMs[ipm] = (PMgroups->isPassive(ipm)) ? 1 : 0;

  if (RecSet == 0) return;
  switch (RecSet->ReconstructionAlgorithm)
    {
    case 0:      
      fByThreshold = RecSet->fCoGIgnoreBySignal;
      fByDistance =  RecSet->fCoGIgnoreFar;
      ThresholdLow = RecSet->CoGIgnoreThresholdLow;
      ThresholdHigh = RecSet->CoGIgnoreThresholdHigh;
      MaxDistance2 = RecSet->CoGIgnoreDistance2;
      break;
    case 1:
      fByThreshold = RecSet->fUseDynamicPassivesSignal;
      fByDistance =  RecSet->fUseDynamicPassivesDistance;
      ThresholdLow = RecSet->SignalThresholdLow;
      ThresholdHigh = RecSet->SignalThresholdHigh;
      MaxDistance2 = RecSet->MaxDistanceSquare;
      CenterOption = RecSet->CGstartOption;
      StartX = RecSet->CGstartX;
      StartY = RecSet->CGstartY;
      break;
    case 2:
      fByThreshold = RecSet->fUseDynamicPassivesSignal;
      fByDistance =  RecSet->fUseDynamicPassivesDistance;
      ThresholdLow = RecSet->SignalThresholdLow;
      ThresholdHigh = RecSet->SignalThresholdHigh;
      MaxDistance2 = RecSet->MaxDistanceSquare;
      CenterOption = RecSet->RMstartOption;
      break;
    case 3:
      //dynamic passives not applicable
      fByThreshold = false;
      fByDistance =  false;
      break;
    case 4:
      {
        QJsonObject json= RecSet->CGonCUDAsettings;
        if (json.contains("Passives")) //old system - compatibility
        {
           bool fOn = false;
           parseJson(json, "Passives", fOn);
           if (!fOn)
           {
               fByThreshold = false;
               fByDistance =  false;
           }
           else
           {
               int PassiveType = 1;
               parseJson(json, "PassiveOption", PassiveType);
               fByThreshold = (PassiveType == 0);
               fByDistance =  (PassiveType == 1);
               parseJson(json, "Threshold", ThresholdLow);
               ThresholdHigh = 1.0e10;
               double MaxDistance = 100.0;
               parseJson(json, "MaxDistance", MaxDistance);
               MaxDistance2 = MaxDistance*MaxDistance;
           }
        }
        else
        {
            fByThreshold = RecSet->fUseDynamicPassivesSignal;
            fByDistance =  RecSet->fUseDynamicPassivesDistance;
            ThresholdLow = RecSet->SignalThresholdLow;
            ThresholdHigh = RecSet->SignalThresholdHigh;
            MaxDistance2 = RecSet->MaxDistanceSquare;
            //CenterOption = RecSet->RMstartOption;
            //parseJson(json, "StartOption", CenterOption);
        }

        parseJson(json, "StartOption", CenterOption);
        parseJson(json, "StartX", StartX);
        parseJson(json, "StartY", StartY);

        //qDebug() << "Dyn pass manager: Cuda dyn passives reports:"<<fByThreshold<<fByDistance<<ThresholdLow<<ThresholdHigh<<MaxDistance2<<CenterOption;
        break;
      }
    case 5:
      //dynamic passives not applicable
      fByThreshold = false;
      fByDistance =  false;
      break;
    default:
      qWarning() << "Unknown reconstruction algorithm!";
      break;
    }
}

void DynamicPassivesHandler::calculateDynamicPassives(int ievent, const AReconRecord* rec)
{
  if (ievent > EventsDataHub->Events.size()-1)
    {
      clearDynamicPassives();
      return;
    }

  double X0=0, Y0=0;
  if (fByDistance)
    {
//      if (CenterOption == 0)
//        {
//          //qDebug()<<"using cog";
//          X0 = rec->xCoG;
//          Y0 = rec->yCoG;
//        }
//      else
//        {
//          //qDebug()<<"using max sig pm";
//          X0 = PMs->X(rec->iPMwithMaxSignal);
//          Y0 = PMs->Y(rec->iPMwithMaxSignal);
//        }

      switch (CenterOption)
      {
      case 0:   //start from CoG data
          X0 = rec->xCoG;
          Y0 = rec->yCoG;
          break;
      case 1:  //starting from XY of the centre of the PM with max signal
          X0 = PMs->X(rec->iPMwithMaxSignal);
          Y0 = PMs->Y(rec->iPMwithMaxSignal);
          break;
      case 2:  // given X and Y
          X0 = StartX;
          Y0 = StartY;
          break;
      case 3:  //starting from scan XY
          X0 = EventsDataHub->Scan[ievent]->Points[0].r[0];
          Y0 = EventsDataHub->Scan[ievent]->Points[0].r[1];
          break;
      default:
          qWarning() << "Unknown center option in dynamic passives";
      }
    }

  for (int ipm=0; ipm < numPMs; ipm++)
    {
      bool fPassive = false;

      if (fByThreshold)
        {
          double gain = PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).gain;
          //double thresholdLow = ThresholdLow*PMs->getGain(ipm);
          double thresholdLow = ThresholdLow * gain;
          //double thresholdHigh = ThresholdHigh*PMs->getGain(ipm);
          double thresholdHigh = ThresholdHigh * gain;
          fPassive = (EventsDataHub->Events.at(ievent).at(ipm) < thresholdLow) || (EventsDataHub->Events.at(ievent).at(ipm) > thresholdHigh);
        }

      if (!fPassive)
        if (fByDistance)
         {
           double dX =  (PMs->X(ipm) - X0);
           double dY =  (PMs->Y(ipm) - Y0);
           double distance2 = dX*dX + dY*dY;
           fPassive = (distance2 > MaxDistance2);
         }

      if (fPassive) setDynamicPassive(ipm);
      else          clearDynamicPassive(ipm);
      //qDebug() << ipm << fPassive;
    }
}

int DynamicPassivesHandler::countActives() const
{
   int actives = 0;
   for (int ipm=0; ipm<numPMs; ipm++)
     if (isActive(ipm)) actives++;
   return actives;
}

void DynamicPassivesHandler::clearDynamicPassives()
{
  for (int ipm=0; ipm<numPMs; ipm++) clearDynamicPassive(ipm);
}
