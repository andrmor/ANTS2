#include "apm.h"
#include "ajsontools.h"

#include <QTextStream>
#include <QDebug>

#include "TH1D.h"
#include "TMath.h"

void APm::saveCoords(QTextStream &file) const
{
    file << x << " " << y << " " << z << "\r\n";
}

void APm::saveCoords2D(QTextStream &file) const
{
    file << x << " " << y << "\r\n";
}

void APm::saveAngles(QTextStream &file) const
{
    file << phi << " " << theta << " " << psi << "\r\n";
}

void APm::preparePHS()
{
  if (SPePHShist)
    {
      delete SPePHShist;
      SPePHShist = 0;
    }

  const int size = SPePHS_x.size();
  if (size < 2) return;

  //SPePHShist = new TH1D("SPePHS"+ipm,"SPePHS", size, SPePHS_x.at(0), SPePHS_x.at(size-1));
  SPePHShist = new TH1D("", "", size, SPePHS_x.at(0), SPePHS_x.at(size-1));
  for (int j = 1; j<size+1; j++) SPePHShist->SetBinContent(j, SPePHS.at(j-1));
  SPePHShist->GetIntegral(); //to make thread safe
}

void APm::clearSPePHSCustomDist()
{
    SPePHS_x.clear();
    SPePHS.clear();
    if (SPePHShist)
    {
        delete SPePHShist;
        SPePHShist = 0;
    }
}

void APm::setADC(double max, int bits)
{
    ADCmax = max;
    ADCbits = bits;
    ADClevels = TMath::Power(2, bits) - 1;
    ADCstep = max / ADClevels;
}

void APm::setElChanSPePHS(const QVector<double>& x, const QVector<double>& y)
{
    SPePHS_x = x;
    SPePHS = y;
}

void APm::scaleSPePHS(double gain)
{
    if (gain == AverageSigPerPhE) return; //nothing to change

    if (fabs(gain) > 1e-20) gain /= AverageSigPerPhE;
    else gain = 0;

    if (SPePHSmode < 3) AverageSigPerPhE = AverageSigPerPhE * gain;
    else if (SPePHSmode == 3)
    {
        //custom SPePHS - have to adjust the distribution
        //QVector<double> x = SPePHS_x;
        //QVector<double> y = SPePHS;
        //for (int ix=0; ix<x.size(); ix++) x[ix] *= gain;
        //setElChanSPePHS(x, y);

        for (double& d : SPePHS_x) d *= gain;
    }
}

void APm::copySPePHSdata(const APm& from)
{
    SPePHSmode       = from.SPePHSmode;
    AverageSigPerPhE = from.AverageSigPerPhE;
    SPePHSsigma      = from.SPePHSsigma;
    SPePHSshape      = from.SPePHSshape;
    SPePHS_x         = from.SPePHS_x;
    SPePHS           = from.SPePHS;

    if (SPePHShist)
    {
        delete SPePHShist; SPePHShist = 0;
    }
    if (from.SPePHShist) SPePHShist = new TH1D(*from.SPePHShist);
}

void APm::copyMCcrosstalkData(const APm& from)
{
    MCcrosstalk   = from.MCcrosstalk;
    MCmodel       = from.MCmodel;
    MCtriggerProb = from.MCtriggerProb;
}

void APm::copyElNoiseData(const APm& from)
{
    ElNoiseSigma = from.ElNoiseSigma;
}

void APm::copyADCdata(const APm& from)
{
    ADCmax    = from.ADCmax;
    ADCbits   = from.ADCbits;
    ADCstep   = from.ADCstep;
    ADClevels = from.ADClevels;
}

void APm::resetOverrides()
{
    effectivePDE = -1.0;
    PDE_lambda.clear();
    PDE.clear();
    PDEbinned.clear();
    AngularSensitivity_lambda.clear();
    AngularSensitivity.clear();
    AngularN1 = 1.0;
    AngularSensitivityCosRefracted.clear();
    AreaSensitivity.clear();
    AreaStepX = 777;
    AreaStepY = 777;
}

void APm::writePHSsettingsToJson(QJsonObject &json) const
{
  json["Mode"]    = SPePHSmode;
  json["Average"] = AverageSigPerPhE;

  switch ( SPePHSmode ) // 0 - use average value; 1 - normal distr; 2 - Gamma distr; 3 - custom distribution
    {
    case 0: break;
    case 1:
      json["Sigma"] = SPePHSsigma;
      break;
    case 2:
      json["Shape"] = SPePHSshape;
      break;
    case 3:
      {
        QJsonArray ar;
        writeTwoQVectorsToJArray(SPePHS_x, SPePHS, ar);
        json["Distribution"] = ar;
        break;
      }
    default: qWarning() << "Unknown SPePHS mode";
    }
}

void APm::readPHSsettingsFromJson(QJsonObject &json)
{
  parseJson(json, "Mode",    SPePHSmode);
  parseJson(json, "Average", AverageSigPerPhE);
  parseJson(json, "Sigma",   SPePHSsigma);
  parseJson(json, "Shape",   SPePHSshape);

  SPePHS_x.clear();
  SPePHS.clear();
  if (json.contains("Distribution"))
    {
      QJsonArray ar = json["Distribution"].toArray();
      readTwoQVectorsFromJArray(ar, SPePHS_x, SPePHS);
    }
  preparePHS();
}
