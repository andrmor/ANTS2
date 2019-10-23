#include "pmsensorgroup.h"
#include "sensorlocalcache.h"
#include "lrf2.h"
#include "apmhub.h"
#include "jsonparser.h"
#include "lrfaxial.h"
#include "lrfcaxial.h"
#include "lrfxy.h"
#include "lrfxyz.h"
#include "lrfaxial3d.h"
#include "lrfcomposite.h"
#include "lrfsliced3d.h"

#include <stdio.h>
#include <math.h>

#include <QVector>
#include <vector>
#include <QDebug>

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif

PMsensorGroup::PMsensorGroup() : lrf(0)
{
}

PMsensorGroup::~PMsensorGroup()
{
  clear();
}

void PMsensorGroup::clear()
{
    if (lrf) delete lrf;
    lrf = 0;
    for(unsigned int i = 0; i < sensors.size(); i++)
        sensors[i].SetLRF(0);
}

PMsensorGroup &PMsensorGroup::operator=(const PMsensorGroup &copy)
{
//  qDebug() << "copy start";
//  qDebug() << "  orignal lrf:"<<copy.lrf;
//  if (copy.lrf) qDebug() << "    type:"<<copy.lrf->type();

  if (copy.lrf) lrf = LRFfactory::copyLRF(copy.lrf);
  else lrf = 0;

//  qDebug() << "  new lrf"<<lrf;
//  if (lrf) qDebug() <<"   type:"<<lrf->type();

  sensors = copy.sensors;
  for (unsigned int i=0; i<sensors.size(); i++) sensors[i].SetLRF(lrf);
//  if (sensors.size()>0) qDebug() << "  sensor 0 lrf:"<<sensors[0].GetLRF();
//  qDebug() << "copy done";
  return *this;
}

QVector<PMsensorGroup> PMsensorGroup::mkFromSensorsAndLRFs(QVector<int> gids, QVector<PMsensor> sensors, QVector<LRF2*> lrfs)
{
    int maxGid = 0;
    for(int i = 0; i < sensors.size(); i++)
    {
        if(gids[i] > maxGid)
            maxGid = gids[i];
    }

    QVector<PMsensorGroup> groups(maxGid+1);
    for(int i = 0; i < sensors.size(); i++)
        groups[gids[i]].sensors.push_back(sensors[i]);

    for(int i = 0; i < lrfs.size(); i++)
        groups[i].setLRF(lrfs[i]);
    return groups;
}

// helper functions to make groups
static double getPhiDetCenter(double x, double y, double flip_y = 1.0)
{
    double phi = atan2(y*flip_y, x);
    // atan2 maps into [-pi;pi], make it [0;2*pi]
    if (phi < -1.0e-6) phi += 2.0*M_PI;
    return phi;
}

static QVector< QPair <double, int> > getGroupByPhi(const APmHub *PMs, QVector< QPair <double, int> > &snake, double rTolerance = 0.1)
{
    QVector< QPair <double, int> > group;
    if (snake.size() == 0) return group;
    double r = snake[0].first;
    do
    {
        int ipmt = snake[0].second;
        group.push_back(qMakePair(getPhiDetCenter(PMs->X(ipmt), PMs->Y(ipmt)), ipmt));
        snake.pop_front();
        if (snake.size() == 0) break;
    } while (fabs(snake[0].first - r) < rTolerance);
    qSort(group);
    return group;
}

QVector<PMsensorGroup> PMsensorGroup::mkIndividual(const APmHub *PMs)
{
    QVector<PMsensorGroup> SensorGroups;

    for (int ipm = 0; ipm < PMs->count(); ipm++)
    {
        PMsensorGroup newGroup;
        newGroup.addSensor(PMs, ipm, 0.0, false);
        SensorGroups.push_back(newGroup);
    }
    SensorGroups.squeeze();
    return SensorGroups;
}

QVector<PMsensorGroup> PMsensorGroup::mkCommon(const APmHub *PMs)
{
    QVector<PMsensorGroup> SensorGroups(1);
    SensorGroups[0] = PMsensorGroup();
    for (int ipm = 0; ipm < PMs->count(); ipm++)
        SensorGroups[0].addSensor(PMs, ipm, -getPhiDetCenter(PMs->X(ipm), PMs->Y(ipm)), false);
        //SensorGroups[0].addSensor(PMs, ipm, 0, false);
    SensorGroups.squeeze();
    return SensorGroups;
}

QVector<PMsensorGroup> PMsensorGroup::mkAxial(const APmHub *PMs)
{
    QVector<PMsensorGroup> SensorGroups;
    QVector< QPair<double, int> > snake = PMs->getPMsSortedByR();
    QVector< QPair<double, int> > group;

    while ((group = getGroupByPhi(PMs, snake)).size() > 0)
    {
        PMsensorGroup newGroup;
        for (int i = 0; i < group.size(); i++)
            newGroup.addSensor(PMs, group[i].second, -group[i].first, false);
        SensorGroups.push_back(newGroup);
    }
    SensorGroups.squeeze();
    return SensorGroups;
}

QVector<PMsensorGroup> PMsensorGroup::mkLattice(const APmHub *PMs, int N_gon)
{
    QVector<PMsensorGroup> result;
    QVector< QPair<double, int> > snake = PMs->getPMsSortedByR();
    QVector< QPair<double, int> > group;

    while ((group = getGroupByPhi(PMs, snake)).size() > 0)
    {
        while (group.size() > 0) {
            PMsensorGroup newGroup;
            double phi0 = group[0].first;
            qSort(group.begin(), group.end(), qGreater< QPair<double, int> >()); // sort in reverse order
            for (int i = group.size()-1; i>=0; i--) // this way we can remove elements on the go - VS
            {
                int ipm = group[i].second;
                double phi = group[i].first - phi0;
                double crap;
                if (modf(phi/(M_PI*2./N_gon)+0.00005, &crap) < 0.0001) {
                    newGroup.addSensor(PMs, ipm, -phi, false);
                    group.remove(i);
                } else {
                    phi = getPhiDetCenter(PMs->X(ipm), PMs->Y(ipm), -1.) - phi0;
                    if (modf(phi/(M_PI*2./N_gon)+0.00005, &crap) < 0.0001) {
                        newGroup.addSensor(PMs, ipm, phi , true);
                        group.remove(i);
                    }
                }
            }
            result.push_back(newGroup);
        }
    }
    result.squeeze();
    return result;
}

QVector<PMsensorGroup> PMsensorGroup::mkHexagonal(const APmHub *PMs)
{
    return mkLattice(PMs, 6);
}

QVector<PMsensorGroup> PMsensorGroup::mkSquare(const APmHub *PMs)
{
    return mkLattice(PMs, 4);
}

/*
QVector<PMsensorGroup> PMsensorGroup::mkHexagonal(const pms *PMs)
{
    QVector<PMsensorGroup> result;
    QVector< QPair<double, int> > snake = PMs->getPMsSortedByR();
    QVector< QPair<double, int> > group;

    while ((group = getGroupByPhi(PMs, snake)).size() > 0)
    {
        PMsensorGroup newGroup;
        double phi0 = group[0].first;
        for (int i = 0; i < group.size(); i++)
        {
            int ipm = group[i].second;
            double phi = group[i].first - phi0;
            double crap;
            if (modf(phi/(M_PI/3)+0.00005, &crap) < 0.0001)
                newGroup.addSensor(PMs, ipm, -phi, false);
            else
                newGroup.addSensor(PMs, ipm, getPhiDetCenter(PMs->X(ipm), PMs->Y(ipm), -1.)-phi0 , true);
        }
        result.push_back(newGroup);
    }
    result.squeeze();
    return result;
}

QVector<PMsensorGroup> PMsensorGroup::mkSquare(const pms *PMs)
{
    QVector<PMsensorGroup> result;
    QVector< QPair<double, int> > snake = PMs->getPMsSortedByR();
    QVector< QPair<double, int> > group;

    while ((group = getGroupByPhi(PMs, snake)).size() > 0)
    {       
        PMsensorGroup newGroup;
        double phi0 = group[0].first;
        for (int i = 0; i < group.size(); i++)
        {
            int ipm = group[i].second;
            double phi = group[i].first - phi0;
            double crap;
            if (modf(phi/(M_PI/2)+0.00005, &crap) < 0.0001)
                newGroup.addSensor(PMs, ipm, -phi, false);
            else
                newGroup.addSensor(PMs, ipm, getPhiDetCenter(PMs->X(ipm), PMs->Y(ipm))-phi0, true);
        }
        result.push_back(newGroup);
    }
    result.squeeze();
    return result;
}
*/
void PMsensorGroup::setLRF(LRF2 *lrf)
{
    this->lrf = lrf;
    for(unsigned int i = 0; i < sensors.size(); i++)
        sensors[i].SetLRF(lrf);
}

void PMsensorGroup::setGains(const double *gains)
{
    if(!gains) return;
    for(unsigned int i = 0; i < sensors.size(); i++)
      sensors[i].SetGain(gains[i]);
}

void PMsensorGroup::writeJSON(QJsonObject &json) const
{
  //qDebug() << "----group save:";
  QJsonArray sensArr_json;
  for (unsigned int isensor=0; isensor<sensors.size(); isensor++)
    {
      //qDebug() << "------sensor " << isensor;
      QJsonObject sens_json;
      sensors[isensor].writeJSON(sens_json);
      sensArr_json.append(sens_json);
    }
  json["sensors"] = sensArr_json;
  //qDebug() << "----- all sensors saved";

  QJsonObject lrf_json;
  lrf->writeJSON(lrf_json);
  json["LRF"] = lrf_json;
  //qDebug() << "----LRF saved";
}

bool PMsensorGroup::readJSON(QJsonObject &json)
{
  JsonParser parser(json);
  QJsonArray sensArr_json;

  //reading sensors
  if (! parser.ParseObject("sensors", sensArr_json) ) return false;
  QVector <QJsonObject> sens_json;
  if (parser.ParseArray(sensArr_json, sens_json))
    {
      for (int isens=0; isens<sens_json.size(); isens++)
        {
          PMsensor tmpSens;
          tmpSens.readJSON(sens_json[isens]);
          sensors.push_back(tmpSens);
        }
    }
  else return false;

  //reading lrfs
  QJsonObject lrf_json;
  if (! parser.ParseObject("LRF", lrf_json) ) return false;
  JsonParser lrfparser(lrf_json);
  LRF2* lrf = 0;
  QString type;
  lrfparser.ParseObject("type", type);
  //qDebug() << "LRF type="<< type;

  if      (type == "Axial")      lrf = new LRFaxial(lrf_json);
  else if (type == "Radial")     lrf = new LRFaxial(lrf_json);   // compatibility
  else if (type == "ComprAxial") lrf = new LRFcAxial(lrf_json);
  else if (type == "ComprRad")   lrf = new LRFcAxial(lrf_json);  // compatibility
  else if (type == "Axial3D")    lrf = new LRFaxial3d(lrf_json);
  else if (type == "ComprAxial3D")    lrf = new LRFcAxial3d(lrf_json);
  else if (type == "Radial3D")   lrf = new LRFaxial3d(lrf_json); // compatibility
  else if (type == "XY")         lrf = new LRFxy(lrf_json);
  else if (type == "Freeform")   lrf = new LRFxy(lrf_json);      // compatibility
#ifdef TPS3M
  else if (type == "XYZ")        lrf = new LRFxyz(lrf_json);
#endif
  else if (type == "Composite")  lrf = new LRFcomposite(lrf_json);
  else if (type == "Sliced3D")   lrf = new LRFsliced3D(lrf_json);
  else
    {
      qDebug() << "Unknown LRF type!";
      return false;
    }

  //registering LRF for the group and all sensors of the group
  PMsensorGroup::setLRF(lrf);
  return true;
}

void PMsensorGroup::addSensor(const APmHub *PMs, int ipm, double phi, bool flip)
{
    PMsensor newSensor(ipm);
    newSensor.SetTransform(-PMs->X(ipm), -PMs->Y(ipm), phi, flip);
    sensors.push_back(newSensor);
}
