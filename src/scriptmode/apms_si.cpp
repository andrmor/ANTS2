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

    H["SetAreaResponse"] = "Set override for the area response multiplier of the PM. Response multiplier can be from 0 to 1.0"
            "1st parameter: PM index\n"
            "2nd parameter: matrix of responses multipliers:\n"
            "  [ [R_x1yM, R_x2yM, ... , R_xNyM ], [R_x1yM-1, R_x2yM-1, ... , R_xNyM-1], ..., [R_x1y1, R_xNy1] ]\n"
            "3rd parameter: size of cell in mm in X direction\n"
            "4th parameter: size of cell in mm in Y direction.\n"
            "x and y are indeces of the local coordinates in the PM's frame (0,0 is the center)";

    H["SetAngularResponse"] = "Set angular response multiplier for the PM\n"
            "1st parameter: PM index\n"
            "2nd parameter: matrix of area responses multipliers:\n"
            "[ [0, ResponseAt0], [Angle2, R2], [Angle3, R3], ... , [90, ResponseAt90] ]\n"
            "The response multiplier values will be automatically scaled to have factor of 1.0 at 0 degrees";
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

void APms_SI::SetPDE(int ipm, double PDE)
{
    if (!checkValidPM(ipm)) return;

    if (PDE < 0)
    {
        abort("PDE cannot be negative");
        return;
    }

    PMs->at(ipm).effectivePDE = PDE;
    Config->GetDetector()->writeToJson(Config->JSON);
}

void APms_SI::SetPDEvsWave(int ipm, QVariantList ArrayOf_ArrayWavePDE)
{
    if (!checkValidPM(ipm)) return;

    QString err = "To configure PDE vs wavelength provide an array with elemenets of type [Wavelength, PDE]";
    int size = ArrayOf_ArrayWavePDE.size();
    if (size == 0)
    {
        abort(err);
        return;
    }

    QVector<double> WAVE, PDE;
    for (int i=0; i<size; i++)
    {
        QVariant elv = ArrayOf_ArrayWavePDE[i];
        QVariantList el = elv.toList();
        if (el.size() != 2)
        {
            abort(err);
            return;
        }

        bool bOK1, bOK2;
        double wave = el[0].toDouble(&bOK1);
        double pde  = el[1].toDouble(&bOK2);
        if (!bOK1 || !bOK2)
        {
            abort(err);
            return;
        }
        if (wave < 0 || ( i != 0 && wave <= WAVE[i-1]) )
        {
            abort("Wavelengths should be positive and increasing");
            return;
        }
        if (pde < 0)
        {
            abort("PDE value cannot be negative");
            return;
        }

        WAVE << wave;
        PDE << pde;
    }

    PMs->setPDEwave(ipm, &WAVE, &PDE);
    PMs->RebinPDEsForPM(ipm);

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

void APms_SI::SetAngularResponse(int ipm, QVariantList ArrayOf_ArrayAngleResponse, double refIndex)
{
    if (!checkValidPM(ipm)) return;

    QString err = "To configure angular response provide an array with elemenets of type [Degrees, Response]";
    int size = ArrayOf_ArrayAngleResponse.size();
    if (size == 0)
    {
        abort(err);
        return;
    }

    QVector<double> vAngle, vResponse;

    for (int i=0; i<size; i++)
    {
        QVariant elv = ArrayOf_ArrayAngleResponse[i];
        QVariantList el = elv.toList();
        if (el.size() != 2)
        {
            abort(err);
            return;
        }

        bool bOK1, bOK2;
        double angle     = el[0].toDouble(&bOK1);
        double response  = el[1].toDouble(&bOK2);
        if (!bOK1 || !bOK2)
        {
            abort(err);
            return;
        }
        if (angle < 0 || angle > 90.0  || ( i != 0 && angle <= vAngle[i-1]) )
        {
            abort("Angles should be in range of 0 - 90 and increasing");
            return;
        }
        if (response < 0)
        {
            abort("Response cannot be negative");
            return;
        }

        vAngle    << angle;
        vResponse << response;
    }

    if (vAngle.first() != 0 || vAngle.last() != 90.0)
    {
        abort("Angles should start at 0 and end at 90 degrees");
        return;
    }

    if (vResponse.first() < 1e-10)
    {
        abort("Response for normal incidence cannot be 0 due to auto-scaling to unity");
        return;
    }

    double norm = vResponse[0];
    if (norm != 1.0)
    {
        for (int i=0; i<vAngle.size(); i++)
            vResponse[i] = vResponse[i]/norm;
    }

    PMs->setAngular(ipm, &vAngle, &vResponse);
    PMs->at(ipm).AngularN1 = refIndex;

    PMs->RecalculateAngularForPM(ipm);
    Config->GetDetector()->writeToJson(Config->JSON);
}

void APms_SI::SetAreaResponse(int ipm, QVariantList MatrixOfResponses, double StepX, double StepY)
{
    if (!checkValidPM(ipm)) return;

    if (StepX < 0 || StepY < 0)
    {
        abort("Steps cannot be negative");
        return;
    }

    QString err = "To configure area response provide a 2D array of responses: line is vs X, column is vs Y, start at top-left corner";
    int sizeY = MatrixOfResponses.size();
    if (sizeY == 0)
    {
        abort(err);
        return;
    }

    int sizeX;
    for (int iy=0; iy<sizeY; iy++)
    {
        QVariant elv = MatrixOfResponses[iy];
        QVariantList el = elv.toList();

        if (iy == 0) sizeX = el.size();
        else if (sizeX != el.size())
        {
            abort("Not matching number of elements per line in area response");
            return;
        }
    }

    QVector< QVector<double> > vResponse;
    vResponse.resize(sizeY);
    for (int iy=0; iy<sizeY; iy++)
        vResponse[iy].resize(sizeX);

    for (int iy=0; iy<sizeY; iy++)
    {
        QVariant elv = MatrixOfResponses[iy];
        QVariantList el = elv.toList();

        for (int ix=0; ix<sizeX; ix++)
        {
            bool bOK;
            double response  = el[ix].toDouble(&bOK);
            if (!bOK || response < 0)
            {
                abort("Responses should be non-negative numeric values");
                return;
            }
            vResponse[ix][sizeY-1-iy] = response;
        }
    }

    PMs->setArea(ipm, &vResponse, StepX, StepY);
    Config->GetDetector()->writeToJson(Config->JSON);
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
