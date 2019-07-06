#include "apms_si.h"
#include "aconfiguration.h"
#include "detectorclass.h"
#include "apmhub.h"
#include "apmposangtyperecord.h"
#include "apmarraydata.h"

#include <limits>

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

APms_SI::APms_SI(AConfiguration *Config)
    : Config(Config), PMs(Config->GetDetector()->PMs)
{
    Description = "Access to PM positions / add PMs or remove all PMs from the configuration";

    H["SetPMQE"] = "Sets the QE of PM. Forces a call to UpdateAllConfig().";
    H["SetElGain"]  = "Set the relative strength for the electronic channel of ipm PMs.\nForces UpdateAllConfig()";

    H["GetElGain"]  = "Get the relative strength for the electronic channel of ipm PMs";

    H["GetRelativeGains"]  = "Get array with relative gains (max scaled to 1.0) of PMs. The factors take into account both QE and electronic channel gains (if activated)";

    H["GetPMx"] = "Return x position of PM.";
    H["GetPMy"] = "Return y position of PM.";
    H["GetPMz"] = "Return z position of PM.";

    H["SetPassivePM"] = "Set this PM status as passive.";
    H["SetActivePM"] = "Set this PM status as active.";

    H["CountPMs"] = "Return number of PMs";
}

bool APms_SI::checkValidPM(int ipm)
{
    if (ipm < 0 || ipm >= PMs->count())
    {
        abort("Wrong PM index: " + ipm);
        return false;
    }
    return true;
}

bool APms_SI::checkAddPmCommon(int UpperLower, int type)
{
    if (UpperLower<0 || UpperLower>1)
      {
        qWarning() << "Wrong UpperLower parameter: 0 - upper PM array, 1 - lower";
        return false;
      }
    if (type<0 || type>PMs->countPMtypes()-1)
      {
        qWarning() << "Attempting to add PM of non-existing type.";
        return false;
      }
    return true;
}

int APms_SI::CountPM() const
{
    return PMs->count();
}

double APms_SI::GetPMx(int ipm)
{
  if (!checkValidPM(ipm)) return std::numeric_limits<double>::quiet_NaN();
  return PMs->X(ipm);
}

double APms_SI::GetPMy(int ipm)
{
  if (!checkValidPM(ipm)) return std::numeric_limits<double>::quiet_NaN();
  return PMs->Y(ipm);
}

double APms_SI::GetPMz(int ipm)
{
  if (!checkValidPM(ipm)) return std::numeric_limits<double>::quiet_NaN();
  return PMs->Z(ipm);
}

bool APms_SI::IsPmCenterWithin(int ipm, double x, double y, double distance_in_square)
{
  if (!checkValidPM(ipm)) return false;
  double dx = x - PMs->at(ipm).x;
  double dy = y - PMs->at(ipm).y;
  return ( (dx*dx + dy*dy) < distance_in_square );
}

bool APms_SI::IsPmCenterWithinFast(int ipm, double x, double y, double distance_in_square) const
{
  double dx = x - PMs->at(ipm).x;
  double dy = y - PMs->at(ipm).y;
  return ( (dx*dx + dy*dy) < distance_in_square );
}

QVariant APms_SI::GetPMtypes()
{
   QJsonObject obj;
   PMs->writePMtypesToJson(obj);
   QJsonArray ar = obj["PMtypes"].toArray();

   QVariant res = ar.toVariantList();
   return res;
}

QVariant APms_SI::GetPMpositions() const
{
    QVariantList arr;
    const int numPMs = PMs->count();
    for (int ipm=0; ipm<numPMs; ipm++)
    {
        QVariantList el;
        el << QVariant(PMs->at(ipm).x) << QVariant(PMs->at(ipm).y) << QVariant(PMs->at(ipm).z);
        arr << QVariant(el);
    }
    //  qDebug() << QVariant(arr);
    return arr;
}

void APms_SI::RemoveAllPMs()
{
   Config->GetDetector()->PMarrays[0] = APmArrayData();
   Config->GetDetector()->PMarrays[1] = APmArrayData();
   Config->GetDetector()->writeToJson(Config->JSON);
   Config->GetDetector()->BuildDetector();
}

bool APms_SI::AddPMToPlane(int UpperLower, int type, double X, double Y, double angle)
{
  if (!checkAddPmCommon(UpperLower, type)) return false;

  APmArrayData& ArrData = Config->GetDetector()->PMarrays[UpperLower];
  //qDebug() << "Size:"<<ArrData.PositionsAnglesTypes.size()<<"Reg:"<<ArrData.Regularity;

  if (ArrData.PositionsAnglesTypes.isEmpty())
    {
      //this is the first PM, can define array regularity
      ArrData.Regularity = 1;
      ArrData.PMtype = type;
    }
  else
    {
      if (ArrData.Regularity != 1)
      {
          qWarning() << "Attempt to add auto-z PM to a regular or full-custom PM array";
          return false;
      }
    }

  ArrData.PositionsAnglesTypes.append(APmPosAngTypeRecord(X, Y, 0, 0,0,angle, type));
  ArrData.PositioningScript.clear();
  ArrData.fActive = true;

  Config->GetDetector()->writeToJson(Config->JSON);
  Config->GetDetector()->BuildDetector();
  return true;
}

bool APms_SI::AddPM(int UpperLower, int type, double X, double Y, double Z, double phi, double theta, double psi)
{
    if (!checkAddPmCommon(UpperLower, type)) return false;

    APmArrayData& ArrData = Config->GetDetector()->PMarrays[UpperLower];
    //qDebug() << "Size:"<<ArrData.PositionsAnglesTypes.size()<<"Reg:"<<ArrData.Regularity;

    if (ArrData.PositionsAnglesTypes.isEmpty())
      {
        //this is the first PM, can define array regularity
        ArrData.Regularity = 2;
        ArrData.PMtype = type;
      }
    else
      {
        if (ArrData.Regularity != 2)
        {
            qWarning() << "Attempt to add full-custom position PM to a auto-z PM array";
            return false;
        }
      }

    ArrData.PositionsAnglesTypes.append(APmPosAngTypeRecord(X, Y, Z, phi,theta,psi, type));
    ArrData.PositioningScript.clear();
    ArrData.fActive = true;

    Config->GetDetector()->writeToJson(Config->JSON);
    Config->GetDetector()->BuildDetector();
    return true;
}

void APms_SI::SetAllArraysFullyCustom()
{
    for (int i=0; i<Config->GetDetector()->PMarrays.size(); i++)
        Config->GetDetector()->PMarrays[i].Regularity = 2;
    Config->GetDetector()->writeToJson(Config->JSON);
}

void APms_SI::SetPDE_factor(int ipm, double value)
{
    if (!checkValidPM(ipm)) return;

    if (value < 0) abort("Cannot use negative PDE factor");
    else
    {
        PMs->at(ipm).relQE_PDE = value;
        Config->GetDetector()->writeToJson(Config->JSON);
        //Config->GetDetector()->BuildDetector();
    }
}

void APms_SI::SetSPE_factor(int ipm, double value)
{
    if (!checkValidPM(ipm)) return;

    if (value < 0) abort("Cannot use negative PDE factor");
    else
    {
        PMs->at(ipm).relElStrength = value;
        Config->GetDetector()->writeToJson(Config->JSON);
        //Config->GetDetector()->BuildDetector();
    }
}

void APms_SI::SetPDE_factors(QVariant CommonValue_or_Array)
{
    setFactors(CommonValue_or_Array, true);
}

void APms_SI::SetSPE_factors(QVariant CommonValue_or_Array)
{
    setFactors(CommonValue_or_Array, false);
}

void APms_SI::setFactors(QVariant CommonValue_or_Array, bool bDoPDE)
{
    if (CommonValue_or_Array.type() == QVariant::List)
    {
        QVariantList vl = CommonValue_or_Array.toList();
        if (vl.size() < PMs->count())
        {
            abort("Too short array with factors");
            return;
        }

        bool bOK;
        for (int ipm = 0; ipm < PMs->count(); ipm++)
        {
            double val = vl.at(ipm).toDouble(&bOK);
            if (!bOK || val < 0)
            {
                abort("Array of factors should contain only non-negative numeric values");
                return;
            }
        }

        if (bDoPDE)
            for (int ipm = 0; ipm < PMs->count(); ipm++) PMs->at(ipm).relQE_PDE = vl.at(ipm).toDouble();
        else
            for (int ipm = 0; ipm < PMs->count(); ipm++) PMs->at(ipm).relElStrength = vl.at(ipm).toDouble();
    }
    else
    {
        bool bOK;
        const double val = CommonValue_or_Array.toDouble(&bOK);
        if (!bOK)
        {
            abort("Error converting factor to double value");
            return;
        }

        if (bDoPDE)
            for (int ipm = 0; ipm < PMs->count(); ipm++) PMs->at(ipm).relQE_PDE = val;
        else
            for (int ipm = 0; ipm < PMs->count(); ipm++) PMs->at(ipm).relElStrength = val;
    }

    Config->GetDetector()->writeToJson(Config->JSON);
    //Config->GetDetector()->BuildDetector();
}

double APms_SI::GetPDE(int ipm, int WaveIndex, double Angle, double Xlocal, double Ylocal)
{
    if (!checkValidPM(ipm)) return 0;

    Config->UpdateSimSettingsOfDetector();
    return PMs->getActualPDE(ipm, WaveIndex) * PMs->getActualAngularResponse(ipm, cos(Angle)) * PMs->getActualAreaResponse(ipm, Xlocal, Ylocal);
}
