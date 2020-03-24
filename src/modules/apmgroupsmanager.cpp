#include "apmgroupsmanager.h"
#include "apmhub.h"
#include "ajsontools.h"
#include "apmtype.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

APmGroupsManager::APmGroupsManager(APmHub *PMs, QJsonObject *ConfigJSON) : QObject(), PMs(PMs), ConfigJSON(ConfigJSON)
{
   definePMgroup("Default group", true);
   setCurrentGroup(0);
   PassiveStatus = QVector<unsigned char>(PMs->count(), 0);
}

APmGroupsManager::~APmGroupsManager()
{
    deleteAllPMgroups();
}

void APmGroupsManager::setCurrentGroup(int iGroup)
{
    if (iGroup < 0 || iGroup >= Groups.size()) return;
    CurrentGroup = iGroup;
    emit CurrentSensorGroupChanged();  //Gain evaluator gui is listening
}

const QString APmGroupsManager::getGroupName(int iGroup) const
{
    if (iGroup < 0 || iGroup >= Groups.size()) return "";
    return Groups.at(iGroup)->Name;
}

int APmGroupsManager::getGroupByName(QString Name) const
{
    for (int igroup = 0; igroup<Groups.size(); igroup++)
        if (Groups.at(igroup)->Name == Name) return igroup;
    return -1;
}

int APmGroupsManager::definePMgroup(QString Name, bool fSkipJsonUpdate)
{
    bool fFound;
    do
    {
        fFound = false;
        for (int igroup = 0; igroup<Groups.size(); igroup++)
            if (Groups.at(igroup)->Name == Name)
            {
                Name += "1";
                fFound = true;
                break;
            }
    }
    while (fFound);
    APmGroup* ng = new APmGroup(Name);
    ng->PMS.resize(PMs->count());
    Groups.append(ng);

    if (!fSkipJsonUpdate)
    {
        synchronizeJsonArraysWithNumberGroups();
        updateGroupsInGlobalConfig();
    }
    return Groups.size()-1;
}

void APmGroupsManager::clearPMgroups(bool fSkipJsonUpdate)
{
    for (int ig=1; ig<Groups.size(); ig++)
        delete Groups[ig];
    Groups.resize(1);
    Groups.first()->Name = "DefaultGroup";

    Groups.first()->PMS.clear();
    Groups.first()->PMS.resize(PMs->count());

    for (int ipm=0; ipm<PMs->count(); ipm++) addPMtoGroup(ipm, 0);

    if (!fSkipJsonUpdate)
    {
        synchronizeJsonArraysWithNumberGroups();
        updateGroupsInGlobalConfig();
    }
    setCurrentGroup(0);
}

void APmGroupsManager::synchronizeJsonArraysWithNumberGroups()
{
    int numGroups = Groups.size();

    QJsonObject jR = (*ConfigJSON)["ReconstructionConfig"].toObject();
    QJsonArray ar = jR["ReconstructionOptions"].toArray();
    if (ar.size() < numGroups)
    {
        while (ar.size() < numGroups) ar.append(ar.first());
        jR["ReconstructionOptions"] = ar;
        (*ConfigJSON)["ReconstructionConfig"] = jR;
    }
    if (ar.size() > numGroups)
    {
        while (ar.size() > numGroups) ar.removeLast();
        jR["ReconstructionOptions"] = ar;
        (*ConfigJSON)["ReconstructionConfig"] = jR;
    }

    ar = jR["FilterOptions"].toArray();
    if (ar.size() < numGroups)
    {
        while (ar.size() < numGroups) ar.append(ar.first());
        jR["FilterOptions"] = ar;
        (*ConfigJSON)["ReconstructionConfig"] = jR;
    }
    if (ar.size() > numGroups)
    {
        while (ar.size() > numGroups) ar.removeLast();
        jR["FilterOptions"] = ar;
        (*ConfigJSON)["ReconstructionConfig"] = jR;
    }
}

void APmGroupsManager::deleteAllPMgroups()
{
    for (int ig=0; ig<Groups.size(); ig++)
        delete Groups[ig];
    Groups.clear();
}

bool APmGroupsManager::renameGroup(int igroup, QString name)
{
    if (igroup<0 || igroup>Groups.size()-1) return false;
    Groups[igroup]->Name = name;
    updateGroupsInGlobalConfig();
    return true;
}

void APmGroupsManager::autogroup(int Regularity)
{
    int numPMs = PMs->count();
    if (numPMs==0) return;
    int PMtypes = PMs->countPMtypes();

    deleteAllPMgroups();
    if (Regularity)
      { //upper and lower PM planes are definitely defined
        for (int iul = 0; iul <2; iul++) //iul = 0 - upper, 1 - lower
           for (int ityp = 0; ityp<PMtypes; ityp++)
              {
                QVector<int> vp;
                for (int ipm = 0; ipm<numPMs; ipm++)
                  if (PMs->at(ipm).type == ityp && PMs->at(ipm).upperLower == iul) vp << ipm;

                if (!vp.isEmpty())
                  {
                    //PMs found for this location / type
                    QString str;
                    if (iul == 0) str = "UpperPMs_";
                    else str = "LowerPMs_";
                    str += PMs->getType(ityp)->Name;
                    int ThisGroup = definePMgroup(str, true);
                    for (int i=0; i<vp.count(); i++)
                        addPMtoGroup(vp.at(i), ThisGroup);
                  }
              }
      }
    else
      {
        //irregular array or GDML file was loaded
        QList<int> AllPMs;
        AllPMs.append(0);
        for (int ipm=1; ipm<PMs->count(); ipm++)
          {
            for (int i=0; i<AllPMs.size(); i++)
              {
                if (PMs->at(ipm).z > PMs->at(i).z )
                  {
                    AllPMs.insert(i, ipm);
                    break;
                  }
                if (i == AllPMs.size()-1)
                  {
                    //it is the smallest (or the same) Z
                    AllPMs.append(ipm);
                    break;
                  }
              }
          }
        //AllPMs contains the list of all PMs sorted by decreasing Z
        //splitting it in sublists for each PM type
        QVector< QList<int> > PMsByType(PMtypes);
        for (int ityp=0; ityp<PMtypes; ityp++)
          {
            for (int index = 0; index < AllPMs.size(); index++)
              {
                  int ipm = AllPMs[index];
                  if (PMs->at(ipm).type == ityp) PMsByType[ityp].append(ipm);
              }
          }

        int lastGroup = 0;
        //now all PMs are sorted by type and Z
        for (int ityp = 0; ityp<PMtypes; ityp++)
          {
            double lastZ = 1e10;
            for (int index = 0; index < PMsByType[ityp].size(); index++)
              {
                int ipm = PMsByType[ityp][index];
                int thisZ = PMs->at(ipm).z;
                if (thisZ != lastZ)
                  {
                    //new group
                    lastGroup = definePMgroup("Z="+QString::number(thisZ) + "_" + PMs->getType(ityp)->Name, true);
                  }
                //add pm to this group
                addPMtoGroup(ipm, lastGroup);
                lastZ = thisZ;
              }
          }
      }

     if (countPMgroups() == 0) clearPMgroups();
     else
     {
         updateGroupsInGlobalConfig();
         synchronizeJsonArraysWithNumberGroups();
         setCurrentGroup(0);
     }
}

bool APmGroupsManager::removeGroup(int igroup)
{
    if (igroup<0 || igroup>Groups.size()-1) return false;
    if (Groups.size()<2) return false;
    delete Groups[igroup];
    Groups.remove(igroup);

    synchronizeJsonArraysWithNumberGroups();
    updateGroupsInGlobalConfig();

    if (CurrentGroup == igroup) setCurrentGroup(0);
    return true;
}

bool APmGroupsManager::isPmBelongsToGroup(int ipm, int igroup) const
{
    if (igroup < 0 || igroup >= Groups.size()) return false;
    if (ipm < 0 || ipm >= PMs->count()) return false;
    if (Groups.at(igroup)->PMS.isEmpty()) return false;

    return Groups.at(igroup)->PMS.at(ipm).member;
}

int APmGroupsManager::countPMsWithoutGroup(QVector<int>* unassigned) const
{
    int counter = 0;
    for (int ipm=0; ipm<PMs->count(); ipm++)
    {
        bool fFound = false;
        for (int ig=0; ig<Groups.count(); ig++)
            if (Groups[ig]->isPmMember(ipm))
            {
                fFound = true;
                break;
            }
        if (!fFound)
        {
            counter++;
            if (unassigned) *unassigned << ipm;
        }
    }
    return counter;
}

int APmGroupsManager::countStaticPassives()
{
    //quick patch for assert failure loading neutron Anger camera - 5.03.2020
    if (PassiveStatus.size() != PMs->count())
        PassiveStatus = QVector<unsigned char>(PMs->count(), 0);

    int counter = 0;
    for (int ipm=0; ipm<PMs->count(); ipm++)
        if (isStaticPassive(ipm)) counter++;
    return counter;
}

void APmGroupsManager::writeSensorGroupsToJson(QJsonObject &json)
{
    // group info + pm-related
    QJsonArray ar;
    for (int ig=0; ig<Groups.size(); ig++)
    {
       QJsonObject jg;
       Groups[ig]->writeToJson(jg);
       ar << jg;
    }
    json["SensorGroups"] = ar;

    QJsonArray passives;
        for (int ipm=0; ipm<PassiveStatus.size(); ipm++)
           passives.append( isStaticPassive(ipm) );
    json["StaticPassives"] = passives;
}

bool APmGroupsManager::readGroupsFromJson(QJsonObject &json)
{
    //assuming json to be ReconstructionConfig json
    int oldCurrent = CurrentGroup;
    clearPMgroups(true);

    if (json.contains("SensorGroups")) //else old system, limited compatibility provided
    {
        QJsonArray ar = json["SensorGroups"].toArray();
        if (ar.size()<1)
          {
              qWarning() << "Error: Empty array of Group sensors in json";
              return false;
          }

        deleteAllPMgroups();
        for (int ig=0; ig<ar.size(); ig++)
        {
            QJsonObject jgr = ar[ig].toObject();
            APmGroup* g = new APmGroup();
            g->readFromJson(jgr, PMs->count());
            Groups.append(g);
        }
    }
    else  qWarning("Error: there is no SensorGroups object in json");

    if (oldCurrent < Groups.size()) setCurrentGroup(oldCurrent);

    PassiveStatus.clear();
    if (json.contains("StaticPassives"))
    {
        QJsonArray passives = json["StaticPassives"].toArray();
        if (passives.size() == PMs->count())
        {
            for (int ipm=0; ipm<passives.size(); ipm++)
            {
                bool fPas = passives[ipm].toBool(false);
                PassiveStatus << (fPas ? 1 : 0);
            }
        }
    }

    return true;
}

void APmGroupsManager::updateGroupsInGlobalConfig()
{
    QJsonObject obj = (*ConfigJSON)["ReconstructionConfig"].toObject();
    writeSensorGroupsToJson(obj);
    (*ConfigJSON)["ReconstructionConfig"] = obj;
}

void APmGroupsManager::fixSensorGroupsCompatibility(QJsonObject &json)
{
     //GROUPS
     QJsonObject dynPassJson; // in case we need to transfer dyn passives
     if (json.contains("PMrelatedSettings"))
     {
         QJsonObject pmJson = json["PMrelatedSettings"].toObject();

         QVector<double> Min, Max, Gain;
         QVector<QString> Name;
         QVector<int> PMgrouping;

         int numPMs = PMs->count();

         if (pmJson.contains("CutOffs"))
           {
             QJsonArray mima = pmJson["CutOffs"].toArray();
             for (int ipm=0; ipm<mima.size(); ipm++)
             {
                     QJsonArray el = mima[ipm].toArray();
                     if (el.size() == 2 )
                     {
                         Min << el[0].toDouble();
                         Max << el[1].toDouble();
                     }
                     else
                     {
                         Min << -1.0e10;
                         Max << 1.0e10;
                         qWarning() << "Rec json compatibility: cutoffs mismatch";
                     }
             }
           }
         if (Min.size()!=numPMs || Max.size()!=numPMs)
         {
             Min = QVector<double>(numPMs, -1.0e10);
             Max = QVector<double>(numPMs, +1.0e10);
         }

         //PM groups - when groups are cleared -> group and relgain are reset at PMs
         if (pmJson.contains("Groups"))
           {
             QJsonObject grJson = pmJson["Groups"].toObject();
             if (grJson.contains("PMgroupDescriptions"))
             {
                 QJsonArray name = grJson["PMgroupDescriptions"].toArray();
                 for (int i=0; i<name.size(); i++)
                     Name << name.at(i).toString();
             }
             if (grJson.contains("PMgrouping"))
             {
                 QJsonArray grouping = grJson["PMgrouping"].toArray();
                 for (int i=0; i<grouping.size(); i++)
                     PMgrouping << grouping.at(i).toInt();
             }
             //old thresholds - using only first!!!
             if (!json.contains("DynamicPassives")) //only if new system is not in json
             {
                 dynPassJson["IgnoreBySignal"] = false;
                 dynPassJson["SignalLimitLow"] = -1.0e10;
                 dynPassJson["SignalLimitHigh"] = 1.0e10;
                 dynPassJson["IgnoreByDistance"] = false;
                 dynPassJson["DistanceLimit"] = 1.0e10;
                 if (grJson.contains("DynamicPassiveRanges"))
                 {
                     QJsonArray ar = grJson["DynamicPassiveRanges"].toArray();
                     if (ar.size()>0)
                     {
                           double range = ar.at(0).toDouble(100);
                           //qDebug() << "Range:"<<range;
                           dynPassJson["DistanceLimit"] = range;

                     }
                 }
                 if (grJson.contains("DynamicPassiveThreshold"))
                 {
                     QJsonArray ar = grJson["DynamicPassiveThreshold"].toArray();
                     if (ar.size()>0)
                     {
                           double limit = ar.at(0).toDouble(0);
                           //qDebug() << "Thr:"<<limit;
                           dynPassJson["SignalLimitLow"] = limit;
                     }
                 }
             }
           }
         if (Name.size()<1) Name << "DefaultGroup";
         if (PMgrouping.size()!=numPMs) PMgrouping = QVector<int>(numPMs, 0);
         for (int i=0; i<PMgrouping.size(); i++)
         {
             int ig = PMgrouping.at(i);
             if (ig<0 || ig>Name.size()-1) PMgrouping[i]=0;
         }

         if (pmJson.contains("Gains"))
         {
             QJsonArray gains = pmJson["Gains"].toArray();
             for (int i=0; i<gains.size(); i++)
                 Gain << gains.at(i).toDouble();
         }
         if (Gain.size()!=numPMs) Gain = QVector<double>(numPMs, 1.0);

         if (pmJson.contains("StaticPassives"))
           {
             QJsonArray passives = pmJson["StaticPassives"].toArray();
             if (passives.size() == PMs->count())
               {
                 for (int ipm=0; ipm<passives.size(); ipm++)
                     if (passives[ipm].toBool()) setStaticPassive(ipm);
                     else clearStaticPassive(ipm);
               }
             else qWarning() << "Size mismatch: passives array and number of PMs - skipping";
           }

         //creating groups
         deleteAllPMgroups();
         for (int ig=0; ig<Name.size(); ig++)
             definePMgroup(Name[ig], true);
         //populating groups
         for (int ipm=0; ipm<PMs->count() && ipm<PMgrouping.size(); ipm++)
         {
             int igroup = PMgrouping[ipm];
             if (igroup<Groups.size())
                 //PMgroups[igroup]->Pms.append(APmRecord(ipm, Gain[ipm], Min[ipm], Max[ipm]));
                 addPMtoGroup(ipm, igroup, APmRecordNew(true, Gain[ipm], Min[ipm], Max[ipm]));
         }
         setCurrentGroup(0);
         json.remove("PMrelatedSettings");

         // RECONSTRUCTION OPTIONS
         if (json.contains("ReconstructionOptions"))
           if (json["ReconstructionOptions"].isObject())
            {
                QJsonObject jj = json["ReconstructionOptions"].toObject();
                if (!dynPassJson.isEmpty()) jj["DynamicPassives"] = dynPassJson;
                QJsonArray ar;
                for (int i=0; i<Groups.size(); i++)
                    ar << jj;
                json["ReconstructionOptions"] = ar;
            }

         // FILTER OPTIONS
         if (json.contains("FilterOptions"))
           if (json["FilterOptions"].isObject())
            {
                QJsonObject jj = json["FilterOptions"].toObject();
                QJsonArray ar;
                for (int i=0; i<Groups.size(); i++)
                    ar << jj;
                json["FilterOptions"] = ar;
            }

         writeSensorGroupsToJson(json);
     }
}

void APmGroup::writeToJson(QJsonObject &json)
{
    json["Name"] = Name;

    QJsonArray ar;
      for (int i=0; i<PMS.size(); i++)
        if (PMS.at(i).member) ar << i;
    json["PmIndexes"] = ar;

    ar = QJsonArray();
      for (int i=0; i<PMS.size(); i++)
        if (PMS.at(i).member) ar << PMS.at(i).gain;
    json["Gains"] = ar;

    ar = QJsonArray();
      for (int i=0; i<PMS.size(); i++)
        if (PMS.at(i).member)
        {
            QJsonArray el;
            el << PMS.at(i).cutOffMin << PMS.at(i).cutOffMax;
            ar << el;
        }
    json["CutOffs"] = ar;
}

bool APmGroup::readFromJson(QJsonObject &json, int numPMs)
{
   PMS.clear();
   PMS.resize(numPMs);

   if (!json.contains("Name"))
   {
       qWarning() << "--- json does not contain sensor group record in new format";
       return false;
   }

   parseJson(json, "Name", Name);
   QJsonArray arIndex;
   parseJson(json, "PmIndexes", arIndex);
   QJsonArray arGains;
   parseJson(json, "Gains", arGains);
   QJsonArray arCutOffs;
   parseJson(json, "CutOffs", arCutOffs);

   if (arIndex.size() != arGains.size() ||  arIndex.size() != arCutOffs.size())
   {
       qWarning() << "json record of PM sensor group: array size mismatch!";
       return false;
   }

   for (int i=0; i<arIndex.size(); i++)
   {
       int ipm = arIndex.at(i).toInt();
       if (ipm<0 || ipm>numPMs-1)
       {
           qWarning() << "json record of PM sensor group: bad pm index"<<ipm;
           PMS.clear();
           PMS.resize(numPMs);
           return false;
       }
       double Gain = arGains.at(i).toDouble();
       QJsonArray el = arCutOffs.at(i).toArray();
       if (el.size() != 2)
       {           
           qWarning() << "json record of PM sensor group: cutoffs should contain two doubles";
           PMS.clear();
           PMS.resize(numPMs);
           return false;
       }
       double Min = el.at(0).toDouble();
       double Max = el.at(1).toDouble();
       PMS[ipm] = APmRecordNew(true, Gain, Min, Max);
   }
   return true;
}

bool APmGroup::isPmMember(int ipm) const
{
    if (ipm>PMS.size()-1) return false;
    return PMS.at(ipm).member;
}

void APmGroup::onNumberPMsChanged(int numPMs)
{
    PMS.resize(numPMs);
}

void APmGroupsManager::selectActiveSensorGroup(int igroup)
{
  for (int ipm=0; ipm<PassiveStatus.size(); ipm++)
    if (Groups.at(igroup)->PMS.at(ipm).member) clearTMPpassive(ipm);
      else setTMPpassive(ipm);
}

void APmGroupsManager::clearActiveSensorGroups()
{
  for (int ipm=0; ipm<PMs->count(); ipm++)
      clearTMPpassive(ipm);
}

void APmGroupsManager::addPMtoGroup(int ipm, int iGroup, APmRecordNew record)
{
    if (ipm<0 || ipm>PMs->count()-1) return;
    if (iGroup<0 || iGroup>Groups.size()-1) return;
    Groups[iGroup]->PMS[ipm] = record;
}

void APmGroupsManager::removeAllPmRecords(int iGroup)
{
    if (iGroup<0 || iGroup>countPMgroups()-1) return;
    Groups[iGroup]->PMS.clear();
    Groups[iGroup]->PMS.resize(PMs->count());
    updateGroupsInGlobalConfig();
}

void APmGroupsManager::removePMfromGroup(int ipm, int iGroup)
{
    if (ipm<0 || ipm>PMs->count()-1) return;
    if (iGroup<0 || iGroup>Groups.size()-1) return;
    Groups[iGroup]->PMS[ipm] = APmRecordNew();
}

int APmGroupsManager::countPMsInGroup(int iGroup) const
{
    if (iGroup<0 || iGroup>Groups.size()-1) return 0;
    int counter = 0;
    for (int i=0; i<PMs->count(); i++)
        if (Groups.at(iGroup)->PMS.at(i).member) counter++;
    return counter;
}

void APmGroupsManager::setCutOffs(int ipm, int iGroup, double min, double max, bool fSkipJsonUpdate)
{
    if (ipm<0 || ipm>PMs->count()-1) return;
    if (iGroup<0 || iGroup>Groups.size()-1) return;

    Groups[iGroup]->PMS[ipm].cutOffMin = min;
    Groups[iGroup]->PMS[ipm].cutOffMax = max;

    if (!fSkipJsonUpdate) updateGroupsInGlobalConfig();
}

void APmGroupsManager::setGain(int ipm, int iGroup, double gain, bool fSkipJsonUpdate)
{
    if (ipm<0 || ipm>PMs->count()-1) return;
    if (iGroup<0 || iGroup>Groups.size()-1) return;

    Groups[iGroup]->PMS[ipm].gain = gain;

    if (!fSkipJsonUpdate) updateGroupsInGlobalConfig();
}

void APmGroupsManager::setAllGainsToUnity()
{
    for (int igroup=0; igroup<Groups.size(); igroup++)
        for (int ipm=0; ipm<Groups.at(igroup)->PMS.size(); ipm++)
            Groups[igroup]->PMS[ipm].gain = 1.0;

    updateGroupsInGlobalConfig();
}

double APmGroupsManager::getGain(int ipm, int iGroup) const
{
    if (ipm<0 || ipm>PMs->count()-1) return 0;
    if (iGroup<0 || iGroup>Groups.size()-1) return 0;

    return Groups.at(iGroup)->PMS.at(ipm).gain;
}

double APmGroupsManager::getCutOffMin(int ipm, int iGroup) const
{
    if (ipm<0 || ipm>PMs->count()-1) return 0;
    if (iGroup<0 || iGroup>Groups.size()-1) return 0;

    return Groups.at(iGroup)->PMS.at(ipm).cutOffMin;
}

double APmGroupsManager::getCutOffMax(int ipm, int iGroup) const
{
    if (ipm<0 || ipm>PMs->count()-1) return 0;
    if (iGroup<0 || iGroup>Groups.size()-1) return 0;

    return Groups.at(iGroup)->PMS.at(ipm).cutOffMax;
}

void APmGroupsManager::onNumberOfPMsChanged()
{
    //qDebug() << "Sensor Groups -> triggered onNumberPMsChanged";
    if (Groups.size() == 1)
    {
        //if only one group, it always contain all PMs
        //so reset the group, just keep the name
        QString name = Groups.first()->Name;
        clearPMgroups();
        Groups.first()->Name = name;
    }
    else
    {
        //keep user groups and PM assignment,
        //only remove PMs which no longer exist
        for (int ig=0; ig<Groups.size(); ig++)
            Groups[ig]->onNumberPMsChanged(PMs->count());
    }

    PassiveStatus = QVector<unsigned char>(PMs->count(), 0);
}

int APmGroupsManager::countActives()
{
    int counter=0;
    for (int ipm=0; ipm<PassiveStatus.size(); ipm++)
        if (isActive(ipm)) counter++;
    return counter;
}
