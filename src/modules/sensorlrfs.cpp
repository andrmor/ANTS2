#include "sensorlrfs.h"
#include "jsonparser.h"
#include "pmsensor.h"
#include "pmsensorgroup.h"
#include "ajsontools.h"
#include "eventsdataclass.h"
#include "apmhub.h"
#include "sensorlocalcache.h"
#include "afiletools.h"
#include "amessage.h"

#include "lrfaxial.h"
#include "lrfaxial3d.h"
#include "lrfxy.h"
#include "lrfxyz.h"
#include "lrfcomposite.h"
#include "lrfsliced3d.h"
#include "lrffactory.h"

#include <QDebug>
#include <QVector>
#include <QString>

#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <utility>
#include <QtWidgets/QApplication>
#include <QFileInfo>

static const double Pi = 3.14159265359;

SensorLRFs::SensorLRFs(int nPMs, QObject *parent) :
  QObject(parent)
{
  currentIter = 0;
  nextSerialNumber = 0;
  this->nPMs = nPMs;
  fStopRequest = false;
  //ToleranceR   = 0.1;  //tolerance for  distance (PM groupping) is 0.1 mm for
  //TolerancePhi = 0.02; //tolerance for the angle (PM groupping) is 0.1 mm for

  //qDebug() << "LRF module created with " << nPMs << "sensors";
  LRFmakeJson["AdjustGains"] = true;
  LRFmakeJson["Compression_k"] = 5;
  LRFmakeJson["Compression_lam"] = 50;
  LRFmakeJson["Compression_r0"] = 150;
  LRFmakeJson["DataSelector"] = 0;
  LRFmakeJson["FitOnlyLast"] = false;
  LRFmakeJson["ForceZeroDeriv"] = true;

  LRFmakeJson["ForceNonNegative"] = false;
  LRFmakeJson["ForceNonIncreasingInR"] = false;
  LRFmakeJson["ForceInZ"] = 0;

  LRFmakeJson["GroupToMake"] = 0;
  LRFmakeJson["GroupingOption"] = "Individual";
  LRFmakeJson["GrouppingType"] = 0;
  LRFmakeJson["LRF_3D"] = false;
  LRFmakeJson["LRF_compress"] = true;
  LRFmakeJson["LRF_type"] = 0;
  LRFmakeJson["LimitGroup"] = false;
  LRFmakeJson["Nodes_x"] = 10;
  LRFmakeJson["Nodes_y"] = 10;
  LRFmakeJson["StoreError"] = false;
  LRFmakeJson["UseEnergy"] = true;
  LRFmakeJson["EnergyNormalization"] = 1.0;
  LRFmakeJson["UseGrid"] = true;
  LRFmakeJson["UseGroupping"] = false;
}

SensorLRFs::~SensorLRFs()
{
    // This should no longer be needed, with the PMsensorGroup destructors, but it does no harm anyway.
    for (int i=0; i<iterations.size(); i++)
        iterations[i].clear();
}

void SensorLRFs::clear(int numPMs)
{
  //qDebug() << "--->LRF module: clear";
  for(int i = 0; i < iterations.count(); i++)
      iterations[i].clear();
  iterations.clear();
  currentIter = 0;

  nPMs = numPMs;

  emit SensorLRFsReadySignal(false);
}

bool SensorLRFs::isAllLRFsDefined() const
{
  //current iteration is not set
  if (!currentIter) return false;

  return currentIter->isIterationValid();
/*
  if (!currentIter->isAllLRFsDefined())
    {
      //qDebug is in the function
      return false;
    }

   if (!currentIter->isAllGainsDefined())
     {
       //qDebug is in the function
       return false;
     }

   return true;
*/
}

// =========================================================
// these functions used by ROOT function plotting routines
// r - (x,y) position,
// p[0] - PMT number (passed as double must be converted back to int),
// p[1] - z ()

double SensorLRFs::getFunction(double *r, double *p)
{
  int ipm = (int)(p[0]+0.01);
  return getLRF(ipm, r);
}

double SensorLRFs::getFunction2D(double *r, double *p)
{
  int ipm = (int)(p[0]+0.01);
  return getLRF(ipm, r[0], r[1], p[1]);
}

double SensorLRFs::getInvertedFunction2D(double *r, double *p)
{
  int ipm = (int)(p[0]+0.01);
  return -getLRF(ipm, r[0], r[1], p[1]);
}

double SensorLRFs::getFunction2Dplus(double *r, double *p)
{
  int ipm = (int)(p[0]+0.01);
  int iter = (int)(p[2]+0.01);
  return getLRF(iter, ipm, r[0], r[1], p[1]);
}

double SensorLRFs::getRadial(double *r, double *p)
{
  static const int nPoints = 36;
  int ipm = (int)(p[0]+0.01);
  double rr[3], lrfsum = 0;
  double radius = r[0];
  rr[2] = p[1];
  for(int i = 0; i < nPoints; i++)
  {
      double angle = 2.0*i*Pi/nPoints;
      rr[0] = radius*cos(angle);
      rr[1] = radius*sin(angle);
      lrfsum += getLRFlocal(ipm, rr);
  }
  return lrfsum / nPoints;
}

double SensorLRFs::getRadialPlus(double *r, double *p)
{
  static const int nPoints = 36;
  int ipm = (int)(p[0]+0.01);
  int iter = (int)(p[2]+0.01);
  double rr[3], lrfsum = 0;
  double radius = r[0];
  rr[2] = p[1];
  for(int i = 0; i < nPoints; i++)
  {
      double angle = 2.0*i*Pi/nPoints;
      rr[0] = radius*cos(angle);
      rr[1] = radius*sin(angle);
      lrfsum += getLRFlocal(iter, ipm, rr);
  }
  return lrfsum / nPoints;
}

double SensorLRFs::getError(double *r, double *p)
{
  int ipm = (int)(p[0]+0.01);
  return getLRFErr(ipm, r);
}

double SensorLRFs::getError2D(double *r, double *p)
{
  int ipm = (int)(p[0]+0.01);
  return getLRFErr(ipm, r[0], r[1], p[1]);
}

double SensorLRFs::getErrorRadial(double *r, double *p)
{
  int ipm = (int)(p[0]+0.01);
  double rho[3];
  //rho[0] = PMs->X(ipm)+r[0]; rho[1] = PMs->Y(ipm); rho[2] = p[1];
  rho[0] = p[1]+r[0]; rho[1] = p[2]; rho[2] = p[3];
  return getLRFErrLocal(ipm, rho);
}
// =========================================================

//----------------------------------------------------------
// overloaded functions taking position as (x, y, z)
double SensorLRFs::getLRF(int pmt, double x, double y, double z)
{
  double r[3];
  r[0] = x; r[1] = y; r[2] = z;
  return getLRF(pmt, r);
}

double SensorLRFs::getLRF(int iter, int pmt, double x, double y, double z)
{
  double r[3];
  r[0] = x; r[1] = y; r[2] = z;
  return getLRF(iter, pmt, r);
}

double SensorLRFs::getLRFErr(int pmt, double x, double y, double z)
{
  double r[3];
  r[0] = x; r[1] = y; r[2] = z;
  return getLRFErr(pmt, r);
}

double SensorLRFs::getLRFDrvX(int pmt, double x, double y, double z)
{
  double r[3];
  r[0] = x; r[1] = y; r[2] = z;
  return getLRFDrvX(pmt, r);
}

double SensorLRFs::getLRFDrvY(int pmt, double x, double y, double z)
{
  double r[3];
  r[0] = x; r[1] = y; r[2] = z;
  return getLRFDrvY(pmt, r);
}
//----------------------------------------------------------

void SensorLRFs::addIteration(int pmCount, const QVector<PMsensorGroup> &groups)
{
    iterations.append(IterationGroups(pmCount, groups));    
    currentIter = &iterations.last();
    currentIter->setSerialNumber(nextSerialNumber);
    currentIter->setName("--"+QString::number(nextSerialNumber)+"--");
    nextSerialNumber++;
}

bool SensorLRFs::addNewIteration(IterationGroups &iteration)
{
  if (iteration.isIterationValid())
    {
      iteration.setSerialNumber(nextSerialNumber);
      nextSerialNumber++;
      iteration.setName("--"+QString::number(nextSerialNumber)+"--");      
      iterations.append(iteration);
      //int numIters = iterations.size();
      //iterations.resize(numIters+1);
      //iterations[numIters] = iteration; // using overwriten copy method
      currentIter = &iterations.last();
      return true;
    }
  else return false;
}

bool SensorLRFs::addNewIteration(int pmCount, const QVector<PMsensorGroup> &groups, QString GrouppingOption)
{
   IterationGroups tmp(pmCount, groups);
   tmp.setGrouppingOption(GrouppingOption);

   return SensorLRFs::addNewIteration(tmp);
}

int SensorLRFs::getCurrentIterIndex() const
{
  if (!currentIter) return -1;

  for (int iter = 0; iter<iterations.size(); iter++)
    if (currentIter == &iterations[iter]) return iter;

  return -1;
}

void SensorLRFs::setCurrentIter(int index)
{
    if (index<0 || index>iterations.size()-1)
    {
        qWarning() << "Invalid iteration index!";
        return;
    }
    currentIter = &iterations[index];

    //qDebug() << "--Current have json:--\n"<<currentIter->makeJson;
    if (!currentIter->makeJson.isEmpty())
        LRFmakeJson = currentIter->makeJson;
}

void SensorLRFs::clearHistory(bool onlyTmp)
{
    int indexCur = SensorLRFs::getCurrentIterIndex();
//  qDebug() << "curr:"<<indexCur;

  for (int iter = iterations.size()-1; iter>-1; iter--)
    if (indexCur != iter)
      {
        if (onlyTmp)
          {
            QString kira = iterations[iter].getName();
//            qDebug() << kira << kira.left(2)<<kira.right(2);
            if (kira.left(2) != "--" || kira.right(2) != "--") continue;
          }

//        qDebug() << "removing "<<iter;
        //iterations[iter].clear();
        //iterations.remove(iter);
        SensorLRFs::deleteIteration(iter);
      }

//  qDebug() << "\n size:"<< iterations.size();
  if (iterations.size() == 1) currentIter = &iterations[0];
  else if (iterations.size() == 1) currentIter = 0;
}

void SensorLRFs::deleteIteration(int iter)
{
  if (iter <0 || iter >= iterations.size()) return;

  int curIt = SensorLRFs::getCurrentIterIndex();

  iterations[iter].clear();
  iterations.remove(iter);

  if (iterations.size() == 0) currentIter = 0;
  else
    {
      if (iter == curIt) currentIter = &iterations.last(); //were deleting the current
      else if (iter < curIt) SensorLRFs::setCurrentIter(curIt-1);
  }
}

IterationGroups *SensorLRFs::getIteration(int iter)
{
  if (iter < -1 || iter>=iterations.size()) return 0;
  if (iter == -1) return currentIter;

  return &iterations[iter];
}

int SensorLRFs::getIterationSerialNumer(int iter)
{
  if (iter < -1 || iter>=iterations.size()) return 0;
  if (iter == -1) return currentIter->SerialNumber;

  return iterations[iter].SerialNumber;
}

QString SensorLRFs::getIterationName(int iter)
{
  if (iter < -1 || iter>=iterations.size()) return "invalid";
  if (iter == -1) return currentIter->Name;

  return iterations[iter].Name;
}

bool SensorLRFs::saveAllIterations(QJsonObject &json)
{
  QJsonArray iters;
  for(int i = 0; i < iterations.size(); i++)
    {
        if(!iterations[i].isAllLRFsDefined()) continue;

        QJsonObject iter;
        iterations[i].writeJSON(iter);
        iters.append(iter);
    }
    json["countPMs"] = nPMs;
    json["iterations"] = iters;
    return true;
}

bool SensorLRFs::saveIteration(int iter, QJsonObject &json)
{
  error_string = "";
  if (iterations.isEmpty())
  {
    error_string = "-> No LRF iterations are defined";
    return false;
  }
  if (iter<0 || iter >= iterations.size())
  {
    QTextStream(&error_string)<<"-> Iteration"<<iter<<"is not in valid range of LRF iterations";
    return false;
  }
  if(!iterations[iter].isAllLRFsDefined())
  {
    QTextStream(&error_string)<<"-> In iteration"<<iter<<"not all LRFs are defined";
    return false;
  }

  iterations[iter].writeJSON(json);
  return true;
}

bool SensorLRFs::loadAll(QJsonObject &json)
{
    if (json.isEmpty())
      {
        qWarning() << "json with LRFs is empty!";
        return false;
      }

    if (json.contains("ReconstructionConfig"))
      {
        //will try to extract json from config file
        json = json["ReconstructionConfig"].toObject();
      }

    //could be json extracted from full config file, or it coul dbe a json with reconstruction settings only
    if (json.contains("ActiveLRF")) json = json["ActiveLRF"].toObject();
//    else
//    {
//        qWarning() << "There are no LRF data in this config file!";
//        return false;
//    }

    JsonParser parser(json);
    if(json.contains("sensor_groups"))
      {
        //qDebug() << "Loading - new format!";

        int pms = 0;
        parser.ParseObject("countPMs", pms);
        if (pms != nPMs)
          {
            error_string = "Save file contains "+QString::number(pms) +" sensors, while currently "+QString::number(nPMs) +" sensors are registered";
            return false;
          }

        //save file seems fine, attempting to load
        IterationGroups tmp;
        if (!tmp.readJSON(json))
          {
            qDebug() << "Failed reading iteration json object";
            return false;
          }

        //check, then register and set as current if valid
        QString name = tmp.getName();
        bool OK = SensorLRFs::addNewIteration(tmp);        
        if (OK)
          {
            //qDebug() << "Iteration successfully loaded and registered as current";
            currentIter->setName(name);
          }
        else return false;
      }

    else if (json.contains("iterations"))
    {        
        //loading an array of iterations all at once
        int pms = 0;
        parser.ParseObject("countPMs", pms);
        if (pms != nPMs)
          {
            error_string = "Save file contains "+QString::number(pms) +" sensors, while currently "+QString::number(nPMs) +" sensors are registered";
            return false;
          }
        QJsonArray iters = json["iterations"].toArray();
        int iterSize = iterations.size();
        if(!iters.size())
        {
            error_string = "Save file contains \"iterations\" object, but it's empty";
            return false;
        }
        int validIters = 0;
        int currentIterBak = currentIter ? currentIter - &iterations[0] : -1;
        iterations.resize(iterSize+iters.size());
        currentIter = &iterations[iterSize];
        for(int i = 0; i < iters.size(); i++)
        {
            QJsonObject jsIter = iters[i].toObject();
            if(currentIter->readJSON(jsIter) && currentIter->isAllLRFsDefined())
            {
                currentIter++;
                validIters++;
            }
        }
        if(validIters != iters.size())
            qWarning()<<"On loading all iterations: skipped"<<iters.size()-validIters<<"iterations";
        iterations.resize(iterSize+validIters);
        currentIter = currentIterBak < 0 ? &iterations.last() : &iterations[currentIterBak];
        if(!validIters) return false;
    }
    else
    {
        //message("Save file is incompaible - sensor_groups object not found!");
        //return false;
        //Trying old format
        QJsonArray lrfarr, sensarr;
        QJsonObject countobj;
        int count;

        parser.ParseObject("count", countobj);
        JsonParser parsecnt(countobj);
        parsecnt.ParseObject("sensors", count);
        if (count != nPMs)
        {
            error_string = "Save file contains "+QString::number(count) +" sensors, while currently "+QString::number(nPMs) +" sensors are registered";
            return false;
        }

        //clear(nPMs); ///Raimundo: why was this here? removed
        if (!parser.ParseObject("LRF_block", lrfarr) || !parser.ParseObject("sensor_block", sensarr))
        {
            error_string = "Old format: Either LRFs or Sensor Groups are corrupt";
            return false;
        }

        QVector<QJsonObject> block;
        QVector<LRF2*> lrfs;
        QVector<double> lrfsx;
        QVector<double> lrfsy;
        if (parser.ParseArray(lrfarr, block))
        {
            //qDebug() << "LRF count: " << block.size();
            for (int i=0; i<block.size(); i++)
            {
                QString type = block[i]["type"].toString();
                //qDebug() << "type="<< type;
                //LRF2 *lrf = SensorLRFs::loadJsonLRF(type.toLocal8Bit(), block[i]);
                LRF2 *lrf = LRFfactory::createLRF(type.toLocal8Bit(), block[i]); //using LRFfactory

                if(!lrf)
                {
                    error_string = "Unknown LRF type: "+type;
                    for(int i = 0; i < lrfs.size(); i++)
                        delete lrfs[i];
                    return false;
                }
                if(type == "Composite")
                {
                    error_string = "Old format: Composite is now an incompatible LRF type";
                    //Since no one likes goto's, I have to repeat cleaning code...
                    for(int i = 0; i < lrfs.size(); i++)
                        delete lrfs[i];
                    return false;
                }
                lrfs.append(lrf);
                lrfsx.append(block[i]["xcenter"].toDouble());
                lrfsy.append(block[i]["ycenter"].toDouble());
            }
        }

        QVector<PMsensor> sensors(nPMs);
        QVector<int> gids(nPMs);
        if (parser.ParseArray(sensarr, block))
        {
            if(block.size() != nPMs)
            {
                error_string = "Save file contains "+QString::number(block.size()) +" sensors, while currently "+QString::number(nPMs) +" sensors are registered";
                for(int i = 0; i < lrfs.size(); i++)
                    delete lrfs[i];
                return false;
            }
            for (int i=0; i<block.size() && i<nPMs; i++)
                sensors[i].readOldJSON(block[i], &gids[i], lrfsx.data(), lrfsy.data());
        }
        addIteration(nPMs, PMsensorGroup::mkFromSensorsAndLRFs(gids, sensors, lrfs));
        //currentIter->setGrouppingOption(json["LRF_window"].toObject()["GrouppingType"].toString());
        if (!isAllLRFsDefined())
        {
            if(currentIter) currentIter->clear();
            return false;
        }
    }
/*
  //window settings - corresponds to the settings which were used to make these lrfs
  if (parser.ParseObject("LRF_window", joRecipe))
    {
      qDebug()<<"LRFs: Global Setting loaded successfully";
    }
  else
    {
      qWarning()<<"=> Failed to load global LRF settings!";
      joRecipe = QJsonObject();
    }
*/
    emit SensorLRFsReadySignal(true);    
    //qDebug() << "Load done!";
    return true;
}

//===========================================================================
//--------------------------IterationGroups----------------------------------

IterationGroups::IterationGroups(int pmCount, const QVector<PMsensorGroup> &groups)
{
   PMsensorGroups.resize(groups.size());
   for (int i=0; i<PMsensorGroups.size(); i++)     
       PMsensorGroups[i] = groups[i];

   fillRefs(pmCount);
}

IterationGroups::IterationGroups(const IterationGroups &copy)
{
   *this = copy;
}

IterationGroups &IterationGroups::operator=(const IterationGroups &copy)
{
  Name = copy.Name;
  SerialNumber = copy.SerialNumber;
  GrouppingOption = copy.GrouppingOption;
  makeJson = copy.makeJson;

  PMsensorGroups.resize(copy.PMsensorGroups.size());
  for (int i=0; i< PMsensorGroups.size(); i++)
      PMsensorGroups[i] = copy.PMsensorGroups[i];

  fillRefs(copy.countPMs());
  return *this;
}

void IterationGroups::writeJSON(QJsonObject &json)
{
  json["countPMs"] = countPMs();
  //qDebug() << "Saving groups";
  QJsonArray groupArr;
  for (int igroup=0; igroup<countGroups(); igroup++)
    {
      //qDebug() << "--Saving group "<< igroup;
      QJsonObject js_group;
      js_group["group#"] = igroup;
      getGroup(igroup)->writeJSON(js_group);
      groupArr.append(js_group);
    }
  json["sensor_groups"] = groupArr;
  json["name"] = getName();
  json["groupping_option"] = getGrouppingOption();
  json["makeJson"] = makeJson;
}

bool IterationGroups::readJSON(QJsonObject &json)
{    
  JsonParser parser(json);

  QJsonArray groupArr;
  bool ok = parser.ParseObject("sensor_groups", groupArr);
  if (!ok) return false;

  QVector <QJsonObject> groups_json;
  if (parser.ParseArray(groupArr, groups_json))
    {
      PMsensorGroups = QVector<PMsensorGroup>(groups_json.size());
      for (int igroup=0; igroup<groups_json.size(); igroup++)
          PMsensorGroups[igroup].readJSON(groups_json[igroup]);
    }  
  else
    {
      ///some cleanup here!
      message("Error while loading sensor array!");
      return false;
    }

  IterationGroups::fillRefs();

  QString name;
  if (parser.ParseObject("name", name))
    {
      if (name.startsWith("--"))  name = "--loaded--";    
      setName(name);
    }
  QString option;
  if (parser.ParseObject("groupping_option", option))
    {
      setGrouppingOption(option);
    }
  else
    {
      message("Groupping option NOT loaded!");
    }
  if (json.contains("makeJson"))
      makeJson = json["makeJson"].toObject();

  return true;
}

bool IterationGroups::isIterationValid()
{
  if (!IterationGroups::isAllLRFsDefined())
    {
      //qDebug is in the function
      return false;
    }

   if (!IterationGroups::isAllGainsDefined())
     {
       //qDebug is in the function
       return false;
     }

   return true;
}

bool IterationGroups::isAllLRFsDefined()
{  
    for(int i = 0; i < sensors.size(); i++)
      {
        const LRF2* lrf = sensors[i]->GetLRF();
        if(!lrf)
          {
              qDebug() << "LRF is not defined for PM#: " << sensors.at(i)->GetIndex();
              return false;
          }

//    qDebug() << "VAAALLLIDDDD?"<<lrf->isValid();

        if (!lrf->isValid())
          {
            qDebug() << "LRF reported as invalid for PM#: " << sensors.at(i)->GetIndex();
            return false;
          }
      }

    return true;
}

bool IterationGroups::isAllGainsDefined()
{
  for(int i = 0; i < sensors.size(); i++)
      {
        double gain = sensors.at(i)->GetGain();
        if (gain != gain || gain == 666.0) // nan != nan   and inf != inf
          {
            qDebug() << "Sensor "<< sensors.at(i)->GetIndex() << " has udefined gain!";
            return false;
          }
      }
  return true;
}

void IterationGroups::fillRefs(int pmCount)
{
    sensors.resize(pmCount);
    for (int igroup = 0; igroup < PMsensorGroups.count(); igroup++)
    {
        std::vector<PMsensor> *pms = PMsensorGroups[igroup].getPMs();
        for (unsigned int i = 0; i < pms->size(); i++)
        {
            PMsensor *pm = &(*pms)[i];
            int ipm = pm->GetIndex();
            if (ipm >= sensors.size()) sensors.resize(ipm+1);
            sensors[ipm] = pm;
        }
    }
}

bool SensorLRFs::makeLRFs(QJsonObject &json, EventsDataClass *EventsDataHub, APmHub *PMs)
{
  error_string = "";
  bool ok = LRFsettings.readFromJson(json); //extract setting from json
  if (!ok)
    {
      error_string = "Wrong json";
      return false;
    }
  if (LRFsettings.fLimitGroup)
    {
      if (!getIteration() )
        {
          error_string = "'Only for group' option selected but current iteration does not exist";
          return false;
        }
      if (!getIteration()->isIterationValid() )
        {
          error_string = "'Only for group' option selected but current iteration is not valid";
          return false;
        }
      if (LRFsettings.igrToMake > getIteration()->countGroups()-1)
        {
          error_string = "Group index out of bounds!";
          return false;
        }
    }
  //LRFsettings.dump(); //debug output

  //alias for events
  QVector < QVector <float> > *events = &EventsDataHub->Events;

  //-----------------------------------------------------------------------------

  //making sensor groups
  QVector<PMsensorGroup> *groups;
  QVector<PMsensorGroup> stackGroups;
  if (LRFsettings.fLimitGroup)
    { //use existent groups
      qDebug() << "!--Keeping old groups!";
      groups = getIteration()->getGroups();
      //qDebug()<<"Reusing existent groups => " << groups->size();
      //GrouppingOption = getIteration()->getGrouppingOption();  //cannot be different from the rest! *** !!!
    }
  else
    {
      groups = &stackGroups;
      //create groups
      switch(LRFsettings.GroupingIndex)
        {
        case 0: stackGroups = PMsensorGroup::mkIndividual(PMs); break;
        case 1: stackGroups = PMsensorGroup::mkCommon(PMs); break;
        case 2: stackGroups = PMsensorGroup::mkAxial(PMs); break;
        case 3: stackGroups = PMsensorGroup::mkSquare(PMs); break;
        case 4: stackGroups = PMsensorGroup::mkHexagonal(PMs); break;
        }
      //qDebug()<<"Groups created => " << groups->size();
    }

  //setup data cache for good events
  SensorLocalCache lrfmaker(EventsDataHub->countGoodEvents(),
                            LRFsettings.dataScanRecon,
                            LRFsettings.scale_by_energy,
                            EventsDataHub->ReconstructionData.at(0),
                            &EventsDataHub->Scan,
                            events,
                            &LRFsettings);  
  //qDebug() << "LRFmaker created and initialized";

  //making sensor groups (all or a single one)
  bool OK = true;
  if (LRFsettings.fLimitGroup)
    { //making a single group
      OK = makeGroupLRF(LRFsettings.igrToMake, LRFsettings, groups, &lrfmaker);
    }
  else
    { //making all groups
      int ngrp = groups->size();
      double PrScale = (ngrp>0) ? 100.0/ngrp : 100.0;

      for(int igrp = 0; igrp < ngrp; igrp++)
        {
          qApp->processEvents();
          if (fStopRequest)
            {
              fStopRequest= false;
              error_string = "Aborted by user";
              return false;
            }

          OK = makeGroupLRF(igrp, LRFsettings, groups, &lrfmaker);
          if (!OK) break;

          int progress = PrScale*(igrp+1);
          emit ProgressReport(progress);
        }
    }

  //Attempting to register the created iteration. Sets it as current if valid
  if (OK) OK = addNewIteration(PMs->count(), *groups, LRFsettings.GrouppingOption);

  if (OK)
    {
      //qDebug() << "New current iteration registered";
      currentIter->makeJson = LRFmakeJson;
      emit SensorLRFsReadySignal(true);
    }
  else
    {
      //qDebug() << "LRF make failed!";
      error_string = "Error: failed to make LRFs!";
      emit ProgressReport(0);
    }
  return OK;
}

bool SensorLRFs::makeAxialLRFsFromRfiles(QJsonObject &json, QString FileNamePattern, APmHub *PMs)
{
  bool ok = LRFsettings.readFromJson(json); //extract setting from json
  if (!ok)
    {
      error_string = "Wrong json";
      return false;
    }

  QVector<PMsensorGroup> groups = PMsensorGroup::mkIndividual(PMs);
  for(int igrp = 0; igrp < groups.size(); igrp++)
    {
      QVector<double> ArrR, ArrVal;
      QFileInfo fi(FileNamePattern);
      QString ext = fi.suffix();
      QString base = fi.baseName();
      QString fileName = fi.absolutePath() + "/" +base + QString::number(igrp)+ "."+ext;
      int res = LoadDoubleVectorsFromFile(fileName, &ArrR, &ArrVal);
      if (res != 0)
        {
          error_string = "Failed to read LRF table data from "+fileName;
          qWarning() << error_string;
          return false;
        }

      LRFaxial* newLRF = new LRFaxial(ArrR.last(), LRFsettings.nodesx);
      double result = newLRF->fitRData(ArrR.size(), ArrR.data(), ArrVal.data());
      if (result == -1)
        {
          qWarning() << "LRF fit from table failed!";
          return false;
        }
      groups[igrp].setLRF(newLRF);
    }

  bool OK = addNewIteration(PMs->count(), groups, LRFsettings.GrouppingOption);
  if (OK)
    {
      //qDebug() << "New current iteration registered";
      emit SensorLRFsReadySignal(true);
    }
  else
    {
      //qDebug() << "LRF make failed!";
      error_string = "Error: failed to make LRFs!";
      emit ProgressReport(0);
    }
  return OK;
}

bool SensorLRFs::onlyGains(QJsonObject &json, EventsDataClass *EventsDataHub, APmHub *PMs)
{
  error_string = "";
  bool ok = LRFsettings.readFromJson(json); //extract setting from json
  if (!ok)
    {
      error_string = "Wrong json";
      return false;
    }
  //LRFsettings.dump(); //debug output

  //alias for events
  QVector < QVector <float> > *events = &EventsDataHub->Events;

  //-----------------------------------------------------------------------------

  //already have sensor groups
  QVector<PMsensorGroup> *groups = getIteration()->getGroups();
  //qDebug()<<"Reusing groups => " << groups->size();
  QString GrouppingOption = getIteration()->getGrouppingOption();

  //setup data cache for good events
  //SensorLocalCache lrfmaker(MW->CountGoodReconstructedEvents(), dataScanRecon, scale_by_energy, EventsDataHub->ReconstructionData, MW->Reconstructor->Scan, events);
  SensorLocalCache lrfmaker(EventsDataHub->countGoodEvents(),
                            LRFsettings.dataScanRecon,
                            LRFsettings.scale_by_energy,
                            EventsDataHub->ReconstructionData.at(0),
                            &EventsDataHub->Scan,
                            events,
                            &LRFsettings);
  //lrfmaker.fFitError = LRFsettings.fFitError;
  //lrfmaker.fUseGrid = LRFsettings.fUseGrid;
  //qDebug() << "Group maker created and initialized";

  int ngrp = groups->size();
  double PrScale = (ngrp>0) ? 100.0/ngrp : 100.0;

  for(int igrp = 0; igrp < ngrp; igrp++)
    {
      qApp->processEvents();
      if (fStopRequest)
        {
          fStopRequest= false;
          error_string = "Aborted by user";
          return false;
        }

      PMsensorGroup *group = &(*groups)[igrp];
      //caching data for group processing
      if(!lrfmaker.cacheGroup(group->getPMs(), true, igrp))
      {
        error_string = "Error during caching data!";
        return false;
      }
      //qDebug() << igrp<< "sensor group -> data caching complete";
      group->setGains(lrfmaker.getGains());

      int progress = PrScale*(igrp+1);
      emit ProgressReport(progress);
    }

  //Attempting to register the created iteration. Sets it as current if valid
  bool OK = addNewIteration(PMs->count(), *groups, GrouppingOption);

  if (OK)
    {
      //qDebug() << "New current iteration registered";
      emit SensorLRFsReadySignal(true);
    }
  else
    {
      qWarning() << "gain evaluation failed!";
      error_string = "Error: Gain evaluation failed!";
    }
  return OK;
}

bool SensorLRFs::makeGroupLRF(int igrp, ALrfFitSettings &LRFsettings, QVector<PMsensorGroup> *groups, SensorLocalCache *lrfmaker)
{
  //qDebug() << "Making LRFs for sensor group " << igrp;
  PMsensorGroup *group = &(*groups)[igrp];
  //caching data for group processing
  if(!lrfmaker->cacheGroup(group->getPMs(), LRFsettings.fAdjustGains, igrp)) return false;
  //qDebug() << "Group data caching complete";

  //If we want, we can make multiple different lrfs for the same group
  //this is the right place, now that local data is cached for this group
  //we would of course need to manage the multiple group containers  
  LRF2 *newLRF;
  switch(LRFsettings.LRFtype)
    {
    case 0: newLRF = lrfmaker->mkLRFaxial(LRFsettings.nodesx, LRFsettings.compr); break;
    case 1: newLRF = lrfmaker->mkLRFxy(LRFsettings.nodesx, LRFsettings.nodesy); break;
    case 2: return false; //polar
    case 3: newLRF = lrfmaker->mkLRFcomposite(LRFsettings.nodesx, LRFsettings.nodesy, LRFsettings.compr); break;
    case 4: newLRF = lrfmaker->mkLRFaxial3d(LRFsettings.nodesx, LRFsettings.nodesy, LRFsettings.compr); break;
    case 5: newLRF = lrfmaker->mkLRFsliced3D(LRFsettings.nodesx, LRFsettings.nodesy); break;
    case 6: newLRF = lrfmaker->mkLRFxyz(LRFsettings.nodesx, LRFsettings.nodesy); break;
    }  
  if (!newLRF)
    {
      qWarning() << "LRF creation failed: SVD is singular";
      return false;
    }
  if (LRFsettings.fAdjustGains) group->setGains(lrfmaker->getGains());
  group->setLRF(newLRF);
  //qDebug() << "makeGroupLRF done!";

  return true;
}
