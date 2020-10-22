#include "alrf_si.h"
#include "aconfiguration.h"
#include "eventsdataclass.h"
#include "detectorclass.h"
#include "sensorlrfs.h"

#include <QJsonDocument>
#include <QFile>

ALrf_SI::ALrf_SI(AConfiguration *Config, EventsDataClass *EventsDataHub)
  : Config(Config), EventsDataHub(EventsDataHub) //,f2d(0)
{
  SensLRF = Config->GetDetector()->LRFs;

  Description = "Access to LRFs (B-spline module)";

  H["Make"] = "Calculates new LRFs";

  H["CountIterations"] = "Returns the number of LRF iterations in history.";
  H["GetCurrent"] = "Returns the index of the current LRF iteration.";
  //H["ShowVsXY"] = "Plots a 2D histogram of the LRF. Does not work for 3D LRFs!";
}

QString ALrf_SI::Make()
{
  QJsonObject jsR = Config->JSON["ReconstructionConfig"].toObject();
  SensLRF->LRFmakeJson = jsR["LRFmakeJson"].toObject();
  bool ok = SensLRF->makeLRFs(SensLRF->LRFmakeJson, EventsDataHub, Config->GetDetector()->PMs);
  Config->AskForLRFGuiUpdate();
  if (!ok) return SensLRF->getLastError();
  else return "";
}

double ALrf_SI::GetLRF(int ipm, double x, double y, double z)
{
    //qDebug() << ipm<<x<<y<<z;
    //qDebug() << SensLRF->getIteration()->countPMs();
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRF(ipm, x, y, z);
}

double ALrf_SI::GetLRFerror(int ipm, double x, double y, double z)
{
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRFErr(ipm, x, y, z);
}

QVariant ALrf_SI::GetAllLRFs(double x, double y, double z)
{
    if (!SensLRF->isAllLRFsDefined())
    {
        abort("Not all LRFs are defined!");
        return 0;
    }

    QVariantList arr;
    const int numPMs = SensLRF->getIteration()->countPMs(); //Config->Detector->PMs->count();
    for (int ipm=0; ipm<numPMs; ipm++)
        arr << QVariant( SensLRF->getLRF(ipm, x, y, z) );
    return arr;
}

//void InterfaceToLRF::ShowVsXY(int ipm, int PointsX, int PointsY)
//{
//  int iterIndex = -1;
//  if (!getValidIteration(iterIndex)) return;
//  if (ipm < 0 || ipm >= Config->GetDetector()->PMs->count())
//    return abort("Invalid sensor number "+QString::number(ipm)+"\n");

//  double minmax[4];
//  const PMsensor *sensor = SensLRF->getIteration(iterIndex)->sensor(ipm);
//  sensor->getGlobalMinMax(minmax);

//  if (f2d) delete f2d;
//  f2d = new TF2("f2d", SensLRF, &SensorLRFs::getFunction2D,
//                minmax[0], minmax[1], minmax[2], minmax[3], 2,
//                "SensLRF", "getFunction2D");

//  f2d->SetParameter(0, ipm);
//  f2d->SetParameter(1, 0);//z0);

//  f2d->SetNpx(PointsX);
//  f2d->SetNpy(PointsY);
//  MW->GraphWindow->DrawWithoutFocus(f2d, "surf");
//}

int ALrf_SI::CountIterations()
{
  return SensLRF->countIterations();
}

int ALrf_SI::GetCurrent()
{
  return SensLRF->getCurrentIterIndex();
}

void ALrf_SI::SetCurrent(int iterIndex)
{
  if(!getValidIteration(iterIndex)) return;
  SensLRF->setCurrentIter(iterIndex);
  Config->AskForLRFGuiUpdate();
}

void ALrf_SI::SetCurrentName(QString name)
{
    SensLRF->setCurrentIterName(name);
    Config->AskForLRFGuiUpdate();
}

void ALrf_SI::DeleteCurrent()
{
  int iterIndex = -1;
  if(!getValidIteration(iterIndex)) return;
  SensLRF->deleteIteration(iterIndex);
  Config->AskForLRFGuiUpdate();
}

QString ALrf_SI::Save(QString fileName)
{
  int iterIndex = -1;
  if (!getValidIteration(iterIndex)) return "No data to save";

  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
      return "Cannot open file "+fileName+" to save LRFs!";

  QJsonObject json;
  if (!SensLRF->saveIteration(iterIndex, json))
    {
      saveFile.close();
      return SensLRF->getLastError();
    }

  QJsonDocument saveDoc(json);
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return "";
}

int ALrf_SI::Load(QString fileName)
{
  QFile loadFile(fileName);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    abort("Cannot open save file\n");
    return -1;
  }

  QByteArray saveData = loadFile.readAll();
  loadFile.close();

  QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
  QJsonObject json = loadDoc.object();
  if(!SensLRF->loadAll(json)) {
    abort("Failed to load LRFs: "+SensLRF->getLastError()+"\n");
    return -1;
  }

  //update GUI
  Config->AskForLRFGuiUpdate();

  return SensLRF->getCurrentIterIndex();
}

bool ALrf_SI::getValidIteration(int &iterIndex)
{
  if (iterIndex < -1) {
      abort(QString::number(iterIndex)+" &lt; -1, therefore invalid iteration index.\n");
      return false;
  }
  if (iterIndex == -1)
    iterIndex = SensLRF->getCurrentIterIndex();
  if (iterIndex == -1) {
    abort("There is no LRF data!\n");
    return false;
  }

  int countIter = SensLRF->countIterations();
  if (iterIndex >= countIter) {
    abort("Invalid iteration index "+QString::number(iterIndex)+
          ". There are only "+QString::number(countIter)+" iteration(s).\n");
    return false;
  }

  return true;
}
