#include "alrfmoduleselector.h"
#include "sensorlrfs.h"
#include "ajsontools.h"
#include "apoint.h"
#include "apmhub.h"

#include <QJsonObject>
#include <QDebug>
#include <memory>

#include "TF1.h"
#include "TF2.h"

ALrfModuleSelector::ALrfModuleSelector(APmHub *PMs) : PMs(PMs), OldModule(new SensorLRFs(0))
{
  fOldSelected = true;

  //OldModule = new SensorLRFs(0);
  //qDebug() << "    Created old LRF module";

  //NewModule = new LRF::ARepository();
  //qDebug() << "    Created new LRF module";
}

bool ALrfModuleSelector::isAllLRFsDefined() const
{
    return OldModule->isAllLRFsDefined();
}

bool ALrfModuleSelector::isAllLRFsDefined(bool fUseOldModule) const
{
    return OldModule->isAllLRFsDefined();
}

double ALrfModuleSelector::getLRF(int pmt, const double *r)
{
    return OldModule->getLRF(pmt, r);
}

double ALrfModuleSelector::getLRF(bool fUseOldModule, int pmt, const double *r)
{
    return OldModule->getLRF(pmt, r);
}

double ALrfModuleSelector::getLRF(int pmt, const APoint &pos)
{
    return OldModule->getLRF(pmt, pos.r);
}

double ALrfModuleSelector::getLRF(int pmt, double x, double y, double z)
{
    return OldModule->getLRF(pmt, x, y, z);
}

double ALrfModuleSelector::getLRF(bool fUseOldModule, int pmt, double x, double y, double z)
{
    return OldModule->getLRF(pmt, x, y, z);
}

double ALrfModuleSelector::getLRFErr(int pmt, const double *r)
{
    return OldModule->getLRFErr(pmt, r);
}

double ALrfModuleSelector::getLRFErr(int pmt, const APoint &pos)
{
    return OldModule->getLRFErr(pmt, pos.r);
}

double ALrfModuleSelector::getLRFErr(int pmt, double x, double y, double z)
{
    return OldModule->getLRFErr(pmt, x, y, z);
}

void ALrfModuleSelector::clear(int numPMs)
{
  OldModule->clear(numPMs);
}

void ALrfModuleSelector::saveActiveLRFs_v2(QJsonObject &LRFjson)
{
    int iCur = OldModule->getCurrentIterIndex();
    OldModule->saveIteration(iCur, LRFjson);
}

void ALrfModuleSelector::loadAll_v2(QJsonObject &json)
{
    OldModule->clear(PMs->count());
    OldModule->loadAll(json);
}

QJsonObject ALrfModuleSelector::getLRFmakeJson() const
{
  return OldModule->LRFmakeJson;
}

bool ALrfModuleSelector::isOldSelected() const
{
    return fOldSelected;
}

bool ALrfModuleSelector::isAllSlice3Dold() const
{
    if (fOldSelected)
    {
        for (int ipm=0; ipm<PMs->count(); ipm++)
            if ( (*OldModule)[ipm]->type() != QString("Sliced3D") ) return false;
        return true;
    }
    return false;
}

SensorLRFs *ALrfModuleSelector::getOldModule()
{
    return OldModule.get();
}

TF1 *ALrfModuleSelector::getRootFunctionMainRadial(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{

      const LRF2 *lrf = (*OldModule)[ipm];
      TF1* f = new TF1("f1d", OldModule, &SensorLRFs::getRadial, 0, lrf->getRmax()*1.01, 2, "SensorLRFs", "getRadial");
      f->SetParameter(0, ipm);
      f->SetParameter(1, z);
      return f;

}

TF1 *ALrfModuleSelector::getRootFunctionSecondaryRadial(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{

  int secondIter = -1;
  parseJson(json, "SecondIteration", secondIter);
  if (secondIter == -1) return 0;

  const LRF2 *lrf = OldModule->getIteration(secondIter)->sensor(ipm)->GetLRF();
  TF1* f = new TF1("ff1d", OldModule, &SensorLRFs::getRadialPlus, 0, lrf->getRmax(), 3, "SensorLRFs", "getRadial");
  f->SetParameter(0, ipm);
  f->SetParameter(1, z);
  f->SetParameter(2, secondIter);
  return f;
}

bool ALrfModuleSelector::getNodes(bool fUseOldModule, int ipm, QVector<double> &GrX)
{
      const LRF2 *lrfGen = (*OldModule)[ipm];
      QString type = lrfGen->type();
      if (type != "ComprAxial" && type != "Axial")
         return false;

      LRFaxial* lrf = (LRFaxial*)lrfGen;
      int nodes = lrf->getNint();
      double rmax = lrf->getRmax();

      double Rmin = lrf->Rho(0);
      double Rmax = lrf->Rho(rmax);
      double DX = Rmax-Rmin;

      int lastnode = -1;
      for (int ix=0; ix<102; ix++)
        {
          double x = rmax*ix/100.0;
          double X = lrf->Rho(x);
          int node = X * nodes/DX;
          if (node>lastnode)
            {
              GrX << x;
              lastnode = node;
            }
        }

      return true;
}

TF2 *ALrfModuleSelector::getRootFunctionMainXY(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{

        const PMsensor *sensor = OldModule->getIteration()->sensor(ipm);
        if (!sensor) return 0;
        double minmax[4];
        sensor->getGlobalMinMax(minmax);
        TF2* f2 = new TF2("f2", OldModule, &SensorLRFs::getFunction2D,
                      minmax[0], minmax[1], minmax[2], minmax[3], 2,
                      "SensLRF", "getFunction2D");
        f2->SetParameter(0, ipm);
        f2->SetParameter(1, z);
        return f2;
}

TF2 *ALrfModuleSelector::getRootFunctionSecondaryXY(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{
        int secondIter = -1;
        parseJson(json, "SecondIteration", secondIter);
        if (secondIter == -1) return 0;

        const PMsensor *sensor = OldModule->getIteration()->sensor(ipm);
        if (!sensor) return 0;
        double minmax[4];
        sensor->getGlobalMinMax(minmax);
        TF2* f2 = new TF2("f2bis", OldModule, &SensorLRFs::getFunction2Dplus,
                      minmax[0], minmax[1], minmax[2], minmax[3], 3,
                      "SensLRF", "getFunction2D");
        f2->SetParameter(0, ipm);
        f2->SetParameter(1, z);
        f2->SetParameter(2, secondIter);
        return f2;
}

ALrfModuleSelector ALrfModuleSelector::copyToCurrentThread()
{
  ALrfModuleSelector other(*this);
  return other;
}

