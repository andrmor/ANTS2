#include "alrfmoduleselector.h"
#include "sensorlrfs.h"
#include "modules/lrf_v3/arepository.h"
#include "modules/lrf_v3/corelrfstypes.h"
#include "modules/lrf_v3/alrftypemanager.h"
#include "ajsontools.h"
#include "apmhub.h"

#include <QJsonObject>
#include <QDebug>
#include <memory>

#include "TF1.h"
#include "TF2.h"

ALrfModuleSelector::ALrfModuleSelector(APmHub *PMs) : PMs(PMs),
  OldModule(new SensorLRFs(0)), NewModule(new LRF::ARepository())
{
  fOldSelected = true;
  fUseNewModCopy = false;

  //OldModule = new SensorLRFs(0);
  //qDebug() << "    Created old LRF module";

  //NewModule = new LRF::ARepository();
  //qDebug() << "    Created new LRF module";
}

bool ALrfModuleSelector::isAllLRFsDefined() const
{
  if (fOldSelected) return OldModule->isAllLRFsDefined();
  else return NewModule->isAllLRFsDefined();
}

bool ALrfModuleSelector::isAllLRFsDefined(bool fUseOldModule) const
{
  if (fUseOldModule) return OldModule->isAllLRFsDefined();
  else return NewModule->isAllLRFsDefined();
}

double ALrfModuleSelector::getLRF(int pmt, const double *r)
{
  if (fOldSelected) return OldModule->getLRF(pmt, r);
  else if(fUseNewModCopy) return  (*NewModuleCopy)[pmt].eval(APoint(r));
  else return NewModule->getLRF(pmt, APoint(r));
}

double ALrfModuleSelector::getLRF(bool fUseOldModule, int pmt, const double *r)
{
  if (fUseOldModule) return OldModule->getLRF(pmt, r);
  else if(fUseNewModCopy) return (*NewModuleCopy)[pmt].eval(APoint(r));
  else return NewModule->getLRF(pmt, APoint(r));
}

double ALrfModuleSelector::getLRF(int pmt, const APoint &pos)
{
  if (fOldSelected) return OldModule->getLRF(pmt, pos.r);
  else if(fUseNewModCopy) return (*NewModuleCopy)[pmt].eval(pos);
  else return NewModule->getLRF(pmt, pos);
}

double ALrfModuleSelector::getLRF(int pmt, double x, double y, double z)
{
  if (fOldSelected) return OldModule->getLRF(pmt, x, y, z);
  else if(fUseNewModCopy) return (*NewModuleCopy)[pmt].eval(APoint(x, y, z));
  else return NewModule->getLRF(pmt, APoint(x, y, z));
}

double ALrfModuleSelector::getLRF(bool fUseOldModule, int pmt, double x, double y, double z)
{
  if (fUseOldModule) return OldModule->getLRF(pmt, x, y, z);
  else if(fUseNewModCopy) return (*NewModuleCopy)[pmt].eval(APoint(x, y, z));
  else return NewModule->getLRF(pmt, APoint(x, y, z));
}

double ALrfModuleSelector::getLRFErr(int pmt, const double *r)
{
  if (fOldSelected) return OldModule->getLRFErr(pmt, r);
  else if(fUseNewModCopy) return (*NewModuleCopy)[pmt].sigma(APoint(r));
  else return NewModule->getLRFErr(pmt, APoint(r));
}

double ALrfModuleSelector::getLRFErr(int pmt, const APoint &pos)
{
  if (fOldSelected) return OldModule->getLRFErr(pmt, pos.r);
  else if(fUseNewModCopy) return (*NewModuleCopy)[pmt].sigma(pos);
  else return NewModule->getLRFErr(pmt, pos);
}

double ALrfModuleSelector::getLRFErr(int pmt, double x, double y, double z)
{
  if (fOldSelected) return OldModule->getLRFErr(pmt, x, y, z);
  else if(fUseNewModCopy) return (*NewModuleCopy)[pmt].sigma(APoint(x, y, z));
  else return NewModule->getLRFErr(pmt, APoint(x, y, z));
}

void ALrfModuleSelector::clear(int numPMs)
{
  OldModule->clear(numPMs);
  NewModule->clear(numPMs);
}

void ALrfModuleSelector::saveActiveLRFs_v2(QJsonObject &LRFjson)
{
    int iCur = OldModule->getCurrentIterIndex();
    OldModule->saveIteration(iCur, LRFjson);
}

void ALrfModuleSelector::saveActiveLRFs_v3(QJsonObject &LRFjson)
{
    LRFjson = NewModule->toJson(NewModule->getCurrentRecipeID());
}

void ALrfModuleSelector::loadAll_v2(QJsonObject &json)
{
    OldModule->clear(PMs->count());
    OldModule->loadAll(json);
}

void ALrfModuleSelector::loadAll_v3(QJsonObject &json)
{
    NewModule->clear(PMs->count());
    LRF::ARepository new_repo(json);
    NewModule->mergeRepository(new_repo);
}

QJsonObject ALrfModuleSelector::getLRFmakeJson() const
{
  return OldModule->LRFmakeJson;
}

QJsonObject ALrfModuleSelector::getLRFv3makeJson() const
{
  return NewModule->getNextUpdateConfig();
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

LRF::ARepository *ALrfModuleSelector::getNewModule()
{
  return NewModule.get();
}

TF1 *ALrfModuleSelector::getRootFunctionMainRadial(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{
  if (fUseOldModule)
    {
      const LRF2 *lrf = (*OldModule)[ipm];
      TF1* f = new TF1("f1d", OldModule, &SensorLRFs::getRadial, 0, lrf->getRmax()*1.01, 2, "SensorLRFs", "getRadial");
      f->SetParameter(0, ipm);
      f->SetParameter(1, z);
      return f;
    }
  else
    return getRootFunctionRadialNewModule(NewModule->getCurrentLrfs(), ipm, z, json);
}

TF1 *ALrfModuleSelector::getRootFunctionSecondaryRadial(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{
  if (fUseOldModule)
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
  else
    return getRootFunctionRadialNewModule(NewModule->getSecondaryLrfs(), ipm, z, json);
}

bool ALrfModuleSelector::getNodes(bool fUseOldModule, int ipm, QVector<double> &GrX)
{
  if (fUseOldModule)
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
  else
    {
      //NewModule
      //Doesn't make sense at all for new module.
      //TODO: put nodes viewer in Bspline instructions (shows data as well)
         ///Andr: yes, it does make sense if user asks to draw only one layer of LRF...
      return false;
    }
}

TF2 *ALrfModuleSelector::getRootFunctionMainXY(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{
    if (fUseOldModule)
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
    else
      return getRootFunctionXYNewModule(NewModule->getCurrentLrfs(), ipm, z, json);
}

TF2 *ALrfModuleSelector::getRootFunctionSecondaryXY(bool fUseOldModule, int ipm, double z, QJsonObject &json)
{
    if (fUseOldModule)
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
    else
      return getRootFunctionXYNewModule(NewModule->getSecondaryLrfs(), ipm, z, json);
}

ALrfModuleSelector ALrfModuleSelector::copyToCurrentThread()
{
  ALrfModuleSelector other(*this);
  if(!fOldSelected) {
    other.fUseNewModCopy = true;
    other.NewModuleCopy = std::shared_ptr<const LRF::ASensorGroup>(NewModule->getCurrentLrfs().copyToCurrentThread());
  }
  return other;
}

TF1* ALrfModuleSelector::getRootFunctionRadialNewModule(const LRF::ASensorGroup &lrfs, int ipm, double z, QJsonObject &json)
{
  const int nPoints = 36;
  const LRF::ASensor *lrf_ptr = lrfs.getSensor(ipm);
  if(lrf_ptr == nullptr) return nullptr;
  LRF::ASensor lrf = *lrf_ptr;
  APm &PM = PMs->at(ipm);
  double pm_x = PM.x;
  double pm_y = PM.y;
  double rmin, rmax;
  lrf.getAxialRange(rmin, rmax);

  //Layer selection
  if(!json["AllLayers"].toBool()) {
    int layer = json["Layer"].toInt();
    if(0 <= layer && (unsigned)layer < lrf.deck.size()) {
      LRF::ASensor::Parcel p = lrf.deck[layer];
      lrf.deck.clear();
      lrf.deck.push_back(p);
    }
    else lrf.deck.clear(); //invalid layer...
  }

  TF1* f = new TF1("f1d", [=](double *r, double *) {
    const double radius = r[0];
    const double angle_step = 2*3.141592653589793238462643383279/nPoints;
    double lrfsum = 0;
    APoint pos(0, 0, z);

    for(int i = 0; i < nPoints; i++) {
        double angle = i*angle_step;
        pos.x() = pm_x + radius*cos(angle);
        pos.y() = pm_y + radius*sin(angle);
        lrfsum += lrf.eval(pos);
    }
    return lrfsum / nPoints;
  },
  rmin*0.99, rmax*1.01, 0);
  return f;
}

TF2 *ALrfModuleSelector::getRootFunctionXYNewModule(const LRF::ASensorGroup &lrfs, int ipm, double z, QJsonObject &json)
{
  const LRF::ASensor *lrf_ptr = lrfs.getSensor(ipm);
  if(lrf_ptr == nullptr) return nullptr;
  LRF::ASensor lrf = *lrf_ptr;

  double xmin, xmax, ymin, ymax;
  lrf.getXYRange(xmin, xmax, ymin, ymax);

  //Layer selection
  if(!json["AllLayers"].toBool()) {
    int layer = json["Layer"].toInt();
    if(0 <= layer && (unsigned)layer < lrf.deck.size()) {
      LRF::ASensor::Parcel p = lrf.deck[layer];
      lrf.deck.clear();
      lrf.deck.push_back(p);
    }
    else lrf.deck.clear(); //invalid layer...
  }

  return new TF2("f2d", [=](double *r, double *) {
    return lrf.eval(APoint(r[0], r[1], z));
  },
  xmin, xmax, ymin, ymax, 0, "");
}
