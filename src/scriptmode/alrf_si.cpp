#include "alrf_si.h"
#include "aconfiguration.h"
#include "eventsdataclass.h"
#include "detectorclass.h"
#include "alrfmoduleselector.h"
#include "sensorlrfs.h"

#include <QJsonDocument>

ALrf_SI::ALrf_SI(AConfiguration *Config, EventsDataClass *EventsDataHub)
  : Config(Config), EventsDataHub(EventsDataHub) //,f2d(0)
{
  SensLRF = Config->GetDetector()->LRFs->getOldModule();

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

// ---------------------------------------------------------------------
// --------------------- Raimundo's LRF module -------------------------
// ---------------------------------------------------------------------

#include "apmhub.h"
#include "modules/lrf_v3/arepository.h"
#include "modules/lrf_v3/asensor.h"
#include "modules/lrf_v3/ainstructioninput.h"

ALrfRaim_SI::ALrfRaim_SI(DetectorClass *Detector, EventsDataClass *EventsDataHub) :
  Detector(Detector), EventsDataHub(EventsDataHub), repo(Detector->LRFs->getNewModule())
{
    Description = "Access to new LRF module";
}

QString ALrfRaim_SI::Make(QString name, QVariantList instructions, bool use_scan_data, bool fit_error, bool scale_by_energy)
{
  if(instructions.length() < 1)
    return "No instructions were given";

  if(EventsDataHub->Events.isEmpty())
    return "There are no events loaded";

  if(use_scan_data) {
    if(EventsDataHub->Scan.isEmpty())
      return "Scan data is not setup!";
  } else if(!EventsDataHub->isReconstructionReady())
    return "Reconstruction data is not setup!";

  std::vector<LRF::AInstructionID> current_instructions;
  for(const QVariant &variant : instructions) {
    QJsonObject json = QJsonObject::fromVariantMap(variant.toMap());
    LRF::AInstruction *instruction = LRF::AInstruction::fromJson(json);
    if(instruction == nullptr)
      return "Failed to process instruction: "+QJsonDocument(json).toJson();
    current_instructions.push_back(repo->addInstruction(std::unique_ptr<LRF::AInstruction>(instruction)));
  }

  LRF::ARecipeID recipe;
  bool is_new_recipe = !repo->findRecipe(current_instructions, &recipe);
  if(is_new_recipe)
    recipe = repo->addRecipe(name.toLocal8Bit().data(), current_instructions);

  std::vector<APoint> sensorPos;
  APmHub *PMs = Detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    APm &PM = PMs->at(i);
    sensorPos.push_back(APoint(PM.x, PM.y, PM.z));
  }

  Detector->Config->AskChangeGuiBusy(true);

  LRF::AInstructionInput data(repo, sensorPos, Detector->PMgroups,
                         EventsDataHub, use_scan_data, fit_error, scale_by_energy);

  if(!repo->updateRecipe(data, recipe)) {
    if(is_new_recipe) {
      repo->remove(recipe);
      repo->removeUnusedInstructions();
    }
    return "Failed to update recipe!";
  }
  else repo->setCurrentRecipe(recipe);

  Detector->Config->AskChangeGuiBusy(false);

  return "";  //empty = no error message
}

QString ALrfRaim_SI::Make(int recipe_id, bool use_scan_data, bool fit_error, bool scale_by_energy)
{
  LRF::ARecipeID rid(recipe_id);
  if(!repo->hasRecipe(rid))
    return "Non-existing recipe id";

  if(EventsDataHub->Events.isEmpty())
    return "There are no events loaded";

  if(use_scan_data) {
    if(EventsDataHub->Scan.isEmpty())
      return "Scan data is not setup";
  } else if(!EventsDataHub->isReconstructionReady())
    return "Reconstruction data is not setup";

  std::vector<APoint> sensorPos;
  APmHub *PMs = Detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    APm &PM = PMs->at(i);
    sensorPos.push_back(APoint(PM.x, PM.y, PM.z));
  }

  Detector->Config->AskChangeGuiBusy(true);

  LRF::AInstructionInput data(repo, sensorPos, Detector->PMgroups,
                         EventsDataHub, use_scan_data, fit_error, scale_by_energy);

  if(!repo->updateRecipe(data, rid))
    return "Failed to update recipe!";
  else repo->setCurrentRecipe(rid);

  Detector->Config->AskChangeGuiBusy(false);

  return "";  //empty = no error message
}

int ALrfRaim_SI::GetCurrentRecipeId()
{
  return repo->getCurrentRecipeID().val();
}

int ALrfRaim_SI::GetCurrentVersionId()
{
  return repo->getCurrentRecipeVersionID().val();
}

bool ALrfRaim_SI::SetCurrentRecipeId(int rid)
{
  return repo->setCurrentRecipe(LRF::ARecipeID(rid));
}

bool ALrfRaim_SI::SetCurrentVersionId(int vid, int rid)
{
  if(rid == -1)
    return repo->setCurrentRecipe(repo->getCurrentRecipeID(), LRF::ARecipeVersionID(vid));
  else
    return repo->setCurrentRecipe(LRF::ARecipeID(rid), LRF::ARecipeVersionID(vid));
}

void ALrfRaim_SI::DeleteCurrentRecipe()
{
  repo->remove(repo->getCurrentRecipeID());
}

void ALrfRaim_SI::DeleteCurrentRecipeVersion()
{
  repo->removeVersion(repo->getCurrentRecipeID(), repo->getCurrentRecipeVersionID());
}

double ALrfRaim_SI::GetLRF(int ipm, double x, double y, double z)
{
  const LRF::ASensor *sensor = repo->getCurrentLrfs().getSensor(ipm);
  return sensor != nullptr ? sensor->eval(APoint(x, y, z)) : 0;
}

QList<int> ALrfRaim_SI::GetListOfRecipes()
{
  QList<int> recipes;
  for(auto rid : repo->getRecipeList())
    recipes.append(rid.val());
  return recipes;
}

bool ALrfRaim_SI::SaveRepository(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson());
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

bool ALrfRaim_SI::SaveCurrentRecipe(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson(repo->getCurrentRecipeID()));
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

bool ALrfRaim_SI::SaveCurrentVersion(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson(repo->getCurrentRecipeID(), repo->getCurrentRecipeVersionID()));
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

QList<int> ALrfRaim_SI::Load(QString fileName)
{
  QFile loadFile(fileName);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    abort("Cannot open save file\n");
    return QList<int>();
  }
  QJsonObject json = QJsonDocument::fromJson(loadFile.readAll()).object();
  loadFile.close();

  LRF::ARepository new_repo(json);
  QList<int> new_recipes;
  for(auto rid : new_repo.getRecipeList())
    new_recipes.append(rid.val());
  if(repo->mergeRepository(new_repo))
    return new_recipes;
  else return QList<int>();
}
