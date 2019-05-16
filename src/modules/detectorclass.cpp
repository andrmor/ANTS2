#include "detectorclass.h"
#include "apmhub.h"
#include "apmtype.h"
#include "alrfmoduleselector.h"
#include "amaterialparticlecolection.h"
#include "ajsontools.h"
#include "acommonfunctions.h"
#include "aconfiguration.h"
#include "apreprocessingsettings.h"
#include "apmgroupsmanager.h"
#include "asandwich.h"
#include "aslab.h"
#include "ageoobject.h"
#include "afiletools.h"
#include "modules/lrf_v3/corelrfstypes.h"
#include "modules/lrf_v3/alrftypemanager.h"
#include "modules/lrf_v3/alrftypemanagerinterface.h"

#ifdef SIM
#include "agridelementrecord.h"
#endif

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QPluginLoader> //To load lrf plugins
#include <QtWidgets/QApplication> //To get application path to load plugins

#include "TGeoManager.h"
#include "TRandom2.h"
#include "TGeoPcon.h"
#include "TGeoPgon.h"
#include "TGeoCompositeShape.h"
#include "TNamed.h"

static void autoLoadPlugins() {
  typedef void (*LrfPluginSetupFn)(LRF::ALrfTypeManagerInterface &manager);

  //qDebug() << "Loading plugins from 'plugins' directory";
  QDir plugins_dir(qApp->applicationDirPath());
  if(!plugins_dir.cd("plugins")) {
    qInfo()<<"LRF_v3 plugin loader: Plugin search not performed since '/plugins' directory not found";
    return;
  }

  for(const QString &file_name : plugins_dir.entryList(QDir::Files)) {
    QString full_path = plugins_dir.absoluteFilePath(file_name);
    QLibrary plugin(full_path);
    if(!plugin.load()) {
      qDebug()<<"LRF_v3 plugin loader: failed to open"<<file_name<<"\n";
      qDebug()<<plugin.errorString();
    }
    else {
      auto plugin_setup = (LrfPluginSetupFn)plugin.resolve("Setup");
      if(plugin_setup) {
        plugin_setup(LRF::ALrfTypeManager::instance());
        qDebug()<<"LRF_v3 plugin loader: loaded and configured"<<file_name<<"plugin";
      }
      else
        qDebug()<<"LRF_v3 plugin loader: failed to configure"<<file_name<<"plugin";
    }
  }
}

DetectorClass::DetectorClass(AConfiguration *config)
{
  Config = config;  
  //qDebug() << "--> Detector constructor";  
  RandGen = new TRandom2(QDateTime::currentMSecsSinceEpoch());
  MpCollection = new AMaterialParticleCollection();
  //qDebug() << "    Material collection created";

  GeoManager = 0;  // without it we have a crash in Linux on attempt to remove empty GeoManager: so we create GeoManager first time detector is build
  //qDebug() << "    GeoManager created";

  Sandwich = new ASandwich();
  *Sandwich->DefaultXY = ASlabXYModel(0, 6, 100, 100, 0);
  QObject::connect(MpCollection, SIGNAL(MaterialsChanged(const QStringList)), Sandwich, SLOT(onMaterialsChanged(const QStringList)));

  PMs = new APmHub(MpCollection, RandGen);
  PMs->clearPMtypes();

  LRF::CoreLrfs::Setup(LRF::ALrfTypeManager::instance());
  //qDebug() << "    Initialized new LRF module";
  autoLoadPlugins();

  //qDebug() << "    Photomultipliers manager created";  
  LRFs = new ALrfModuleSelector(PMs);
  //qDebug() << "    LRF selector module created";
  PMgroups = new APmGroupsManager(PMs, &Config->JSON);

  GDML = "";  
  PMarrays.resize(2);
  fSecScintPresent = false;
}

DetectorClass::~DetectorClass()
{
  if (PMgroups) delete PMgroups;
  if (LRFs) delete LRFs;
  //qDebug() << "  --LRFs module deleted";

  if (PMs) delete PMs;
  //qDebug() << "  --Photomultiplier manager deleted";

  if (Sandwich) delete Sandwich;
  //qDebug() << "  --Sandwich deleted";

  if (GeoManager) delete GeoManager;
  //qDebug()<<"  --GeoManager deleted";

  if (MpCollection) delete MpCollection;
  //qDebug() << "  --Material collection deleted";

  delete RandGen;
}

void DetectorClass::writeWorldFixedToJson(QJsonObject &json)
{
  QJsonObject js;
  js["WorldSizeFixed"] = fWorldSizeFixed;
  js["XY"] = WorldSizeXY;
  js["Z"] = WorldSizeZ;
  json["FixedWorldSizes"] = js;
}

bool DetectorClass::MakeDetectorFromJson(QJsonObject &json)
{
  Config->JSON = json;
  return BuildDetector();
}

bool DetectorClass::BuildDetector(bool SkipSimGuiUpdate, bool bSkipAllUpdates)
{
  if (bSkipAllUpdates) SkipSimGuiUpdate = true;

  // qDebug() << "Remake detector triggered"  ;
  if (Config->JSON.isEmpty())
  {
      qCritical() << "!!!Cannot construct detector: Config is empty";
      return false;
  }

  int numPMs = pmCount(); //number of PMs before new detector is constructed

  //qDebug() << "-->Constructing detector using Config json object";
  bool fOK = makeSandwichDetector();
  if (!fOK) return false;

  //handling GDML if present
  QJsonObject js = Config->JSON["DetectorConfig"].toObject();
  if (js.contains("GDML"))
  {
      QString gdml = js["GDML"].toString();
      fOK = importGDML(gdml);  //if failed, it is reported and sandwich is rebuilt
  }

  Config->UpdateSimSettingsOfDetector(); //otherwise some sim data will be lost due to remake of PMs and MPcollection

  if (numPMs != pmCount())
  {
      LRFs->clear(PMs->count()); // clear LRFs if number of PMs changed
      updatePreprocessingAddMultySize();
      PMgroups->onNumberOfPMsChanged();
      if (!bSkipAllUpdates) emit requestGroupsGuiUpdate();
      //emit requestClearPreprocessingSettings();
  }

  //request GUI update
  if (!bSkipAllUpdates)  Config->AskForDetectorGuiUpdate(); //save in batch mode too, just emits a signal
  if (!SkipSimGuiUpdate) Config->AskForSimulationGuiUpdate();
  //emit requestClearEventsData();

  return fOK;
}

bool DetectorClass::BuildDetector_CallFromScript()
{
   writeToJson(Config->JSON);
   return BuildDetector();
}

void DetectorClass::writeToJson(QJsonObject &json)
{
    //qDebug() << "Det->JSON";
    QJsonObject js;

    MpCollection->writeToJson(js);          //Particles+Material (including overrides)
    writeWorldFixedToJson(js);              //Fixed world size - if activated
    Sandwich->writeToJson(js);
    PMs->writePMtypesToJson(js);            //PM array types
    writePMarraysToJson(js);                //PM arrays    
    writeDummyPMsToJson(js);                //dummy PMs
    PMs->writeInividualOverridesToJson(js); //Overrides for individual PMs if defined
    PMs->writeElectronicsToJson(js);        //Electronics
    if (!isGDMLempty())
      writeGDMLtoJson(js);                  //GDML
    writePreprocessingToJson(js);           //Preprocessing for load ascii data

    json["DetectorConfig"] = js;

      //just for control
    //QJsonObject js1;
    //js1["DetectorConfig"] = js;
    //SaveJsonToFile(js1, "DetectorConfig.json");

      //should not be here:
    //Config->AskForDetectorGuiUpdate();
    //Config->AskForSimulationGuiUpdate();
}

void DetectorClass::writePreprocessingToJson(QJsonObject &json)
{
    json["LoadExpDataConfig"] = PreprocessingJson;
}

void DetectorClass::changeLineWidthOfVolumes(int delta)
{
    Sandwich->changeLineWidthOfVolumes(delta);
}

const QString DetectorClass::exportToGDML(const QString& fileName) const
{
    QFileInfo fi(fileName);
    if (fi.suffix().compare("gdml", Qt::CaseInsensitive))
        return "Error: file name should have .gdml extension";

    QByteArray ba = fileName.toLocal8Bit();
    const char *c_str = ba.data();
    GeoManager->SetName("geometry");
    GeoManager->Export(c_str);

    QFile f(fileName);
    if (f.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&f);
        QString txt = in.readAll();

        if (f.remove())
        {
            txt.replace("unit=\"cm\"", "unit=\"mm\"");
            bool bOK = SaveTextToFile(fileName, txt);
            if (bOK) return "";
        }
    }
    return "Error during cm->mm conversion stage!";
}

void DetectorClass::writePMarraysToJson(QJsonObject &json)
{
  QJsonArray arr;
  for (int i=0; i<2;i++)
    {
      QJsonObject js;
      PMarrays[i].writeToJson(js);
      arr.append(js);
    }
  json["PMarrays"] = arr;
}

bool DetectorClass::readPMarraysFromJson(QJsonObject &json)
{
  if (!json.contains("PMarrays"))
    {
      qWarning()<<"Json file does not contain PM arrays data";
      return false;
    }
  /// next two lines: CANNOT do it! Otherwise address of the script for position PMs will be changed -> crash!
  //PMarrays.clear();
  //PMarrays.resize(2); //protection

  QJsonArray arr = json["PMarrays"].toArray();
  if (arr.size() != 2)
    {
      qWarning()<< "Wrong size of PMarrays in Json file";
      return false;
    }
  for (int i=0; i<2;i++)
    {
      QJsonObject js = arr[i].toObject();
      bool fOK = PMarrays[i].readFromJson(js);
      if (!fOK)
        {
          qWarning() << "Error reading PM array #"<<i;
          return false;
        }
    }
  return true;
}

void PMdummyStructure::writeToJson(QJsonObject &json)
{
  QJsonObject js;
  js["Type"] = PMtype;
  js["Array"] = UpperLower;
  QJsonArray arrR;
  arrR.append(r[0]);
  arrR.append(r[1]);
  arrR.append(r[2]);
  js["R"] = arrR;
  QJsonArray arrA;
  arrA.append(Angle[0]);
  arrA.append(Angle[1]);
  arrA.append(Angle[2]);
  js["Angle"] = arrA;
  json["DummyPM"] = js;
}

bool PMdummyStructure::readFromJson(QJsonObject &json)
{
  if (!json.contains("DummyPM"))
    {
      qWarning() << "Not a dummy pm json!";
      return false;
    }
  QJsonObject js = json["DummyPM"].toObject();
  parseJson(js, "Type", PMtype);
  parseJson(js, "Array", UpperLower);
  QJsonArray arrR = js["R"].toArray();
  r[0] = arrR[0].toDouble();
  r[1] = arrR[1].toDouble();
  r[2] = arrR[2].toDouble();
  QJsonArray arrA = js["Angle"].toArray();
  Angle[0] = arrA[0].toDouble();
  Angle[1] = arrA[1].toDouble();
  Angle[2] = arrA[2].toDouble();
  return true;
}

void DetectorClass::writeDummyPMsToJson(QJsonObject &json)
{
  QJsonArray arr;
  for (int i=0; i<PMdummies.size(); i++)
    {
      QJsonObject js;
      PMdummies[i].writeToJson(js);
      arr.append(js);
    }
  json["DummyPMs"] = arr;
}

void DetectorClass::writeGDMLtoJson(QJsonObject &json)
{
  json["GDML"] = GDML;
}

void DetectorClass::constructDetector()
{
  //qDebug() << "--->Creating detector sandwich";

  //qDebug() << "--> Deleting old GeoManager and creating new";
  if (GeoManager) delete GeoManager;
  GeoManager = new TGeoManager();
  GeoManager->SetVerboseLevel(0);

  //qDebug() << "--> Creating materials and media";
  for (int i=0; i<MpCollection->countMaterials(); i++)
    {
      QString tmpStr = (*MpCollection)[i]->name;
      QByteArray ba = tmpStr.toLocal8Bit();
      char *cname = ba.data();
      //(*MpCollection)[i]->GeoMat = new TGeoMaterial(cname, (*MpCollection)[i]->p1, (*MpCollection)[i]->p2, (*MpCollection)[i]->p3);
      (*MpCollection)[i]->generateTGeoMat();
      (*MpCollection)[i]->GeoMed = new TGeoMedium (cname, i, (*MpCollection)[i]->GeoMat);
    }

  //calculate Z of slabs in ASandwich, copy Z edges
  Sandwich->CalculateZofSlabs();
  UpperEdge = Sandwich->Z_UpperBound;
  LowerEdge = Sandwich->Z_LowerBound;

  //qDebug() << "--> Populating PMs module with individual PMs";
  populatePMs();

  if (fWorldSizeFixed)
    {
      //qDebug() << "--> Using user-defined world size";
    }
  else
    {
      //qDebug() << "--> Calculating world size";
      WorldSizeXY = 0;
      WorldSizeZ = 0;
      updateWorldSize(WorldSizeXY, WorldSizeZ);
      WorldSizeXY *= 1.05; WorldSizeZ *= 1.05; //adding margings
    }
  //qDebug() << "    World size XYmax:"<<WorldSizeXY<<"Zmax:"<<WorldSizeZ;

  //qDebug() << "--> Creating top volume";
  top = GeoManager->MakeBox("WorldBox", (*MpCollection)[Sandwich->World->Material]->GeoMed, WorldSizeXY, WorldSizeXY, WorldSizeZ);
  GeoManager->SetTopVolume(top);
  GeoManager->SetTopVisible(true);
  //qDebug() << "--> Positioning sandwich layers";

  //Position WorldTree objects
  QList<APMandDummy> PMsAndDumPms;
  for (int i=0; i<PMs->count(); i++) PMsAndDumPms << APMandDummy(PMs->X(i), PMs->Y(i), PMs->at(i).upperLower);
  for (int i=0; i<PMdummies.size(); i++) PMsAndDumPms << APMandDummy(PMdummies.at(i).r[0], PMdummies.at(i).r[1], PMdummies.at(i).UpperLower);
  Sandwich->clearGridRecords();
  Sandwich->clearMonitorRecords();
  Sandwich->addTGeoVolumeRecursively(Sandwich->World, top, GeoManager, MpCollection, &PMsAndDumPms);

  //for (int i=0; i<Sandwich->GridRecords.size(); i++)
  //    qDebug() << Sandwich->GridRecords[i]->shape<< Sandwich->GridRecords[i]->dx<< Sandwich->GridRecords[i]->dy;

  //qDebug() << "--> Positioning PMs";
  positionPMs();
  if (PMdummies.size()>0)
    {
      //qDebug() << "--> Positioning dummy PMs";
      positionDummies();
    }
  top->SetName("World");
  //qDebug() << "--> Closing geometry";
  GeoManager->CloseGeometry();
  emit newGeoManager(GeoManager);
  //qDebug() << "===> All done!";
}

void DetectorClass::onRequestRegisterGeoManager()
{
    if (GeoManager)
      emit newGeoManager(GeoManager);
}

bool DetectorClass::readDummyPMsFromJson(QJsonObject &json)
{
  if (!json.contains("DummyPMs"))
    {
      //qDebug() << "--- Json does not contain dummy PMs data";
      return false;
    }
  QJsonArray arr = json["DummyPMs"].toArray();
  PMdummies.resize(arr.size());
  for (int i=0; i<arr.size(); i++)
    {
      QJsonObject js = arr[i].toObject();
      PMdummies[i].readFromJson(js);
    }
  //qDebug() << "--> Loaded"<<PMdummies.size()<<"dummy PMs";
  return true;
}

bool DetectorClass::makeSandwichDetector()
{
  //qDebug() << "--->MakeSandwichDetector triggered";
  if (!Config->JSON.contains("DetectorConfig"))
    {
      qCritical() << "Config does NOT contain detector properties!";
      return false;
    }
  QJsonObject js = Config->JSON["DetectorConfig"].toObject();

  MpCollection->readFromJson(js);
  PMs->readPMtypesFromJson(js);
  readPMarraysFromJson(js);  
  PMdummies.clear();
  readDummyPMsFromJson(js);  
  Sandwich->readFromJson(js);
  readWorldFixedFromJson(js);

  //making detector  
  constructDetector();

  //post-construction load
  PMs->readInividualOverridesFromJson(js);
  PMs->readElectronicsFromJson(js);

  top->SetVisContainers(true); //making contaners visible
  return true;
}

void DetectorClass::readWorldFixedFromJson(QJsonObject& json)
{
  QJsonObject jws = json["FixedWorldSizes"].toObject();
  fWorldSizeFixed = false;
  parseJson(jws, "WorldSizeFixed", fWorldSizeFixed);
  if (fWorldSizeFixed)
    {
      parseJson(jws, "XY", WorldSizeXY);
      parseJson(jws, "Z", WorldSizeZ);
    }
}

bool DetectorClass::importGDML(QString gdml)
{
  GDML = gdml;

  bool fOK = processGDML();
  if (fOK)
    {
      qDebug() << "--> GDML successfully registered";
    }
  else
    {
      qWarning() << "||| GDML import failed: "+ErrorString;
      GeoManager = new TGeoManager();
      //GeoManager->AddNavigator(); /// *** !!!
      GDML = "";
      makeSandwichDetector();
    }
  return fOK;
}

int DetectorClass::checkGeoOverlaps()
{
  if (!GeoManager)
    {
      qWarning() << "Attempt to access GeoManager which is NULL";
      return 0;
    }

  double Precision = 0.01; //overlap search precision - in cm

  GeoManager->ClearOverlaps();
  Int_t segments = GeoManager->GetNsegments();

  GeoManager->CheckOverlaps(Precision);
  TObjArray* overlaps = GeoManager->GetListOfOverlaps();
  int overlapCount = overlaps->GetEntries();
  if (overlapCount == 0)
    {
     // Repeating the search with sampling
       //qDebug() << "No overlaps found, checking using sampling method..";
     GeoManager->CheckOverlaps(Precision, "s"); // could be "sd", but the result is the same
     overlaps = GeoManager->GetListOfOverlaps();
     overlapCount = overlaps->GetEntries();
    }

  GeoManager->SetNsegments(segments);  //restore back, get auto reset during the check to some bad default value
  return overlapCount;
}

bool DetectorClass::processGDML()   //if failed, caller have to do: GeoManager = new TGeoManager();GDML = ""; then reconstruct Sandwich Detector
{
  ErrorString = "";
  //have to delete old GeoManager
  if (GeoManager)
    {
      delete GeoManager;
      GeoManager = 0;
    }
  //making tmp file to be used by root
  QFile file("tmpGDML.gdml");
  file.open(QIODevice::WriteOnly);
  if(!file.isOpen())
    {
      ErrorString = "Cannot open file for writing";
      return false;
    }
  QTextStream out(&file);
  out << GDML;
  file.close();
  //importing file
  GeoManager = TGeoManager::Import("tmpGDML.gdml");
  //checking for the result
  if (!GeoManager)
    {
      ErrorString = "Failed to import GDML file!";
      return false;
    }
  qDebug() << "--> GeoManager loaded from GDML file";
  //GeoManager->AddNavigator(); /// *** !!!

  //check validity
  top = GeoManager->GetTopVolume();
    //Correcting NodeNumbers so the PM hodes are properly indexed:
    //after gdml file is loaded, NodeNumber field is set to the number of the volume (0th will be always Top!)
  int totNodes = top->GetNdaughters();
  // qDebug()<<totNodes;
  bool flagFound = false;
  int offsetNumber;
  for (int i=0; i<totNodes; i++)
    {
      TGeoNode* thisNode = (TGeoNode*)top->GetNodes()->At(i);
      QString nodename = top->GetNodes()->At(i)->GetName();
      if (nodename.startsWith("PM"))
        {
          flagFound = true;
          offsetNumber =  thisNode->GetNumber();
          break;
        }      
    }
  if (flagFound)
    {
      //        qDebug()<<"first declared PMs has number of: "<<offsetNumber;
      int counter = 0;
      for (int i=0; i<totNodes; i++)
        {
          TGeoNode* thisNode = (TGeoNode*)top->GetNodes()->At(i);
          QString nodename = top->GetNodes()->At(i)->GetName();
          //            qDebug()<<"-------------loaded:"<<nodename;
          thisNode->GetVolume()->SetTransparency(0);
          thisNode->GetVolume()->SetLineColor(kGray);
          if (nodename.startsWith("PM"))
            {
              thisNode->SetNumber(counter);
              //updating cordinates for this PM
              double Local[3] = {0,0,0};
              double Master[3];
              thisNode->LocalToMaster(Local, Master);
              //qDebug()<<counter<<Master[0]<<Master[1]<<Master[2];
              if (counter > PMs->count()-1)
                {
                  ErrorString = "Too many PMs in the GDML file";
                  return false;
                }
              PMs->at(counter++).setCoords(Master);
              thisNode->GetVolume()->SetLineColor(kGreen);
              thisNode->GetVolume()->SetTitle("P");
            }
          else thisNode->GetVolume()->SetTitle("-");
        }
      //qDebug()<<"In this GDML found "<<counter<<" PMs";
      if (counter != PMs->count())
        {
          ErrorString = "Too few PMs in the GDML file";
          return false;
        }
    }
  else
    {
      ErrorString = "No PMs (name starts from \"PM\" string) were found in the geometry file!";
      //return false;
    }

  //checking materials
  TObjArray* list = GeoManager->GetListOfVolumes();
  int size = list->GetEntries();
  for (int i=0; i<size; i++)
    {
      TGeoVolume* vol = (TGeoVolume*)list->At(i);
      if (!vol) break;
      QString Vname = vol->GetName();
      QString MatName = vol->GetMaterial()->GetName();
      //        qDebug()<<"Volume: "<<Vname<<" Material name:"<<MatName;
      if (Vname.startsWith("PM")) vol->SetLineColor(kGreen);
      else if (Vname.startsWith("Sp")) vol->SetLineColor(kYellow);
      else if (Vname.startsWith("PrScint")) vol->SetLineColor(kRed);
      else if (Vname.startsWith("SecScint")) vol->SetLineColor(kMagenta);
      else if (Vname.startsWith("UpWin")) vol->SetLineColor(kBlue);
      else if (Vname.startsWith("LowWin")) vol->SetLineColor(kBlue);
      else if (Vname.startsWith("Mask")) vol->SetLineColor(40);
      else if (Vname.startsWith("MaskHole")) vol->SetLineColor(40);
      else if (Vname.startsWith("GuideUp")) vol->SetLineColor(38);
      else if (Vname.startsWith("GuideLow")) vol->SetLineColor(38);
      else if (Vname.startsWith("UpperOpening")) vol->SetLineColor(38);
      else if (Vname.startsWith("LowerOpening")) vol->SetLineColor(38);
      else vol->SetLineColor(kGray);
      vol->SetTransparency(0);

      bool foundMaterial = false;
      for (int imat =0; imat<MpCollection->countMaterials(); imat++)
        {
          QString name = (*MpCollection)[imat]->name;
          if (name == MatName)
            {
              foundMaterial = true;
              vol->GetMaterial()->SetIndex(imat);
              //                qDebug()<<"Found! mat id="<<imat;
              break;
            }
        }
      if (!foundMaterial)
        {
          ErrorString = "Found node ("+ Vname+") with unknown material: "+MatName+
                        "\nDefine this material and try import again";
          return false;
        }
    }
  //all is fine!
  checkSecScintPresent();
  return true;
}

void DetectorClass::checkSecScintPresent()
{
  fSecScintPresent = false;
  TObjArray* list = GeoManager->GetListOfVolumes();
  int size = list->GetEntries();
  for (int i=0; i<size; i++)
    {
      TGeoVolume* vol = (TGeoVolume*)list->At(i);
      if (!vol) break;
      QString name = vol->GetName();
      if (name == "SecScint")
        {
          fSecScintPresent = true;
          break;
        }
    }
}

#include "TColor.h"
#include "TColorWheel.h"

void DetectorClass::colorVolumes(int scheme, int id)
{
  //scheme = 0 - default, according to the name
  //scheme = 1 - by material
  //scheme = 2 - medium with id will be red, the rest - black
  TObjArray* list = GeoManager->GetListOfVolumes();
  int size = list->GetEntries();
  for (int i=0; i<size; i++)
    {
      TGeoVolume* vol = (TGeoVolume*)list->At(i);
      if (!vol) break;
      QString name = vol->GetName();
      //      qDebug()<<"name: "<<name<<" medium name: "<<vol->GetMedium()->GetName()<<" med index: "<<vol->GetMedium()->GetId();
      switch (scheme)
        { //default color volumes using name
        case 0:
          {
            vol->SetLineColor(kGray);
            if (name.startsWith("World")) vol->SetLineColor(kGreen);
            if (name.startsWith("PM")) vol->SetLineColor(kGreen);
            if (name.startsWith("dPM")) vol->SetLineColor(30);
            //          if (name.startsWith("Sp")) vol->SetLineColor(28);
            //          if (name.startsWith("PrScint")) vol->SetLineColor(kRed);
            //          if (name.startsWith("SecScint")) vol->SetLineColor(kMagenta);
            //          if (name.startsWith("UpWin")) vol->SetLineColor(kBlue);
            //          if (name.startsWith("LowWin")) vol->SetLineColor(kBlue);
            if (name.startsWith("Mask")) vol->SetLineColor(40);
            if (name.startsWith("MaskHole")) vol->SetLineColor(40);
            if (name.startsWith("LGu")) vol->SetLineColor(38);
            if (name.startsWith("LGl")) vol->SetLineColor(38);
            if (name.startsWith("UOp")) vol->SetLineColor(38);
            if (name.startsWith("LOp")) vol->SetLineColor(38);

//            int slabIndex = Sandwich->FindSlabByName(name);
//            if (slabIndex != -1)
//              {
//                //qDebug() << Sandwich->Slabs.at(slabIndex)->name << Sandwich->Slabs.at(slabIndex)->color;
//                vol->SetLineColor(Sandwich->Slabs.at(slabIndex)->color);
//                vol->SetLineWidth(Sandwich->Slabs.at(slabIndex)->width);
//                vol->SetLineStyle(Sandwich->Slabs.at(slabIndex)->style);
//              }
            const AGeoObject* obj = Sandwich->World->findObjectByName(name);
            if (obj)
            {
                //qDebug() << obj->Name << obj->color;
                vol->SetLineColor(obj->color);
                vol->SetLineWidth(obj->width);
                vol->SetLineStyle(obj->style);
            }
          }
          break;
        case 1:
          { //color by material
            int MatId = vol->GetMaterial()->GetIndex();
            vol->SetLineColor(MatId+1);
            break;
          }
        case 2:  //highlight a given material
          if (vol->GetMaterial()->GetIndex() == id) vol->SetLineColor(kRed);
          else vol->SetLineColor(kBlack);
        }
  }
  emit ColorSchemeChanged(scheme, id);
}

int DetectorClass::pmCount() const
{
  return PMs->count();
}

void DetectorClass::findPM(int ipm, int &ul, int &index)
{
    if (PMarrays[0].fActive)
    {
        if (ipm < PMarrays[0].PositionsAnglesTypes.size())
        {
            ul = 0;
            index = ipm;
            return;
        }
        if (PMarrays[1].fActive)
        {
            index = ipm - PMarrays[0].PositionsAnglesTypes.size();
            if (index < PMarrays[1].PositionsAnglesTypes.size())
            {
                ul = 1;
                return;
            }
        }
    }
    else
    {
        if (PMarrays[1].fActive)
        {
            if (ipm < PMarrays[1].PositionsAnglesTypes.size())
            {
                ul = 1;
                index = ipm;
                return;
            }
        }
    }
    index = -1; //bad ipm
    return;
}

const QString DetectorClass::removePMtype(int itype)
{
    if (PMs->countPMtypes() < 2) return "Cannot remove the last type";

    //no need to check PMarrays -> if type is in use, PMs module will report it
    for (const PMdummyStructure& ad : PMdummies)
        if (ad.PMtype == itype) return "Cannot remove: at least one of the dummy PMs belongs to this type";

    bool bOK = PMs->removePMtype(itype);
    if (!bOK) return "Cannot remove: at least one of the PMs belongs to this type";

    //shifting data in the arrays
    for (APmArrayData& ad : PMarrays)
    {
        if (ad.PMtype >= itype) ad.PMtype--;  // >= i used to shift non-existent type to previous in inactive arrays
        for (APmPosAngTypeRecord& r : ad.PositionsAnglesTypes)
            if (r.type >= itype) r.type--;
    }
    for (PMdummyStructure& ad : PMdummies)
        if (ad.PMtype >= itype) ad.PMtype--;

    return "";
}

TGeoVolume *DetectorClass::generatePmVolume(TString Name, TGeoMedium *Medium, const APmType *tp)
{
  double SizeX = 0.5 * tp->SizeX;
  double SizeY = 0.5 * tp->SizeY;
  double SizeZ = 0.5 * tp->SizeZ;

  //Shape 0 -  box, 1 - cylinder, 2 - polygon, 3 - sphere
  switch (tp->Shape)
    {
    case 0: return GeoManager->MakeBox (Name, Medium, SizeX, SizeY, SizeZ); //box
    case 1: return GeoManager->MakeTube(Name, Medium, 0, SizeX, SizeZ);    //tube
    case 2:
      { //polygon
        TGeoVolume* tgv = GeoManager->MakePgon(Name, Medium, 0, 360.0, 6, 2);
        ((TGeoPcon*)tgv->GetShape())->DefineSection(0, -SizeZ, 0, SizeX);
        ((TGeoPcon*)tgv->GetShape())->DefineSection(1, +SizeZ, 0, SizeX);
        return tgv;
      }
    case 3:
    {
      if (tp->AngleSphere == 90.0 || tp->AngleSphere == 180.0)
          return GeoManager->MakeSphere(Name, Medium, 0, SizeX, 0, tp->AngleSphere);

      double r = tp->getProjectionRadiusSpherical();
      double hHalf = tp->getHalfHeightSpherical();

      TString tubeName = Name + "_tube";
      TGeoVolume* tube = GeoManager->MakeTube(tubeName, Medium, 0, r, hHalf);
      tube->RegisterYourself(); //need?

      TString sphereName = Name + "_sphere";
      TGeoVolume* sphere = GeoManager->MakeSphere(sphereName, Medium, 0, SizeX, 0, (tp->AngleSphere > 90.0 ? 180.0 : tp->AngleSphere) );
      sphere->RegisterYourself(); //need?
      TString transName = Name + "_m";
      TGeoTranslation* tr = new TGeoTranslation(transName, 0, 0, -SizeX + hHalf);
      tr->RegisterYourself();

      TGeoShape* shape = new TGeoCompositeShape(tubeName +"*" + sphereName + ":" + transName);
      return new TGeoVolume(Name, shape, Medium);
    }
    default:
      ErrorString = "Error: unrecognized volume type!";
      qWarning() << ErrorString;
      return 0;
    }
}

void DetectorClass::populatePMs()
{
    PMs->clear();
    for (int ul=0; ul<2; ul++)
    {
        if (PMarrays[ul].fActive)
        {
            // calculate XY positions for regular array and create the record with positions (otherwise already there)
            if (PMarrays[ul].Regularity == 0) calculatePmsXY(ul);
            // calculate Z position for regular and auto-z array
            if (PMarrays[ul].Regularity < 2)
            {
                const int numTypes = PMs->countPMtypes();
                for (int ipm=0; ipm<PMarrays[ul].PositionsAnglesTypes.size(); ipm++)
                {
                    int itype = PMarrays[ul].PositionsAnglesTypes.at(ipm).type;
                    if (itype >= numTypes)
                    {
                        qWarning() << "Unknown type detected, changed to type 0";
                        PMarrays[ul].PositionsAnglesTypes[ipm].type = 0;
                        itype = 0;
                    }

                    const APmType* pType = PMs->getType(itype);
                    double halfWidth = ( pType->Shape == 3 ? pType->getHalfHeightSpherical() : 0.5*pType->SizeZ );
                    double Z = ( ul == 0 ? UpperEdge + halfWidth : LowerEdge - halfWidth );
                    PMarrays[ul].PositionsAnglesTypes[ipm].z = Z;
                }
            }
            //registering PMTs in PMs module
            for (int ipm=0; ipm<PMarrays[ul].PositionsAnglesTypes.size(); ipm++)
                PMs->add(ul, &PMarrays[ul].PositionsAnglesTypes[ipm]);
        }
    }
}

void DetectorClass::positionPMs()
{
  //creating volumes for each type of PMs
  int numPMtypes = PMs->countPMtypes();
  //qDebug()<<"Defined PM types: "<<numPMtypes;
  QVector<TGeoVolume*> pmTypes;
  pmTypes.resize(numPMtypes);
  for (int itype=0; itype<numPMtypes; itype++)
    {
      QString str = "PM" + QString::number(itype);
      QByteArray ba = str.toLocal8Bit();
      char *name = ba.data();
      const APmType *tp = PMs->getType(itype);
      pmTypes[itype] = generatePmVolume(name, (*MpCollection)[tp->MaterialIndex]->GeoMed, tp);
      pmTypes[itype]->SetLineColor(kGreen);
      pmTypes[itype]->SetTitle("P");
    }

  int index = 0;
  for (int ul=0; ul<2; ul++)
    {
      if (PMarrays[ul].fActive)
        {
          //adding PMTs to GeoManager
          for (int i=0; i<PMarrays[ul].PositionsAnglesTypes.size(); i++)
            {
              const APmPosAngTypeRecord *PM = &PMarrays[ul].PositionsAnglesTypes.at(i);
              bool hex = (PMs->getType(PM->type)->Shape == 2);
              TGeoRotation *pmRot = new TGeoRotation("pmRot", PM->phi, PM->theta, PM->psi+(hex ? 90.0 : 0.0));
              pmRot->RegisterYourself();

              //check where is this node. Normal operation if it is World, other wise positioning it inside
              //the found volume              
              TGeoNavigator* navi = GeoManager->GetCurrentNavigator();
              navi->SetCurrentPoint(PM->x, PM->y, PM->z);
              TGeoNode* node = navi->FindNode();
              if (node)
                {
                  TGeoVolume* container = node->GetVolume();
                  TString title = container->GetTitle();
                  if (title != "-")
                    {
                      qWarning() << "PM cannot be placed inside a special volume (PM, dummy PM or optical grid)";
                      ErrorString = "Attempt to place a PM inside a forbidden container";
                    }
                  else if (container == top)
                    {
                      TGeoCombiTrans *pmTrans = new TGeoCombiTrans("pmTrans", PM->x, PM->y, PM->z, pmRot);
                      top->AddNode(pmTypes[PM->type], index, pmTrans);
                    }
                  else
                    {
                      // qDebug() << "PM center detected to be NOT in the World container";
                      // qDebug() << "Detected node name:" << container->GetName();
                      TGeoHMatrix* m = navi->GetCurrentMatrix();
                      TGeoHMatrix minv = m->Inverse();
                      TGeoHMatrix final = minv * TGeoCombiTrans("combined", PM->x, PM->y, PM->z, pmRot);
                      TGeoHMatrix* final1 = new TGeoHMatrix(final);
                      container->AddNode(pmTypes[PM->type], index, final1);
                    }
                }
              else
                {
                  qWarning() << "PM cannot be placed outside of the world";
                  ErrorString = "Attempt to place a PM outside the world";
                }

              index++; //need unique index (lower continues after upper)
            }
        }
    }
}

void DetectorClass::calculatePmsXY(int ul)
{
  PMarrays[ul].PositionsAnglesTypes.clear(); //since its regular, there is no data there

  bool hexagonal = (PMarrays[ul].Packing == 0) ? false : true;
  int iX = PMarrays[ul].NumX;
  int iY = PMarrays[ul].NumY;
  double CtC = PMarrays[ul].CenterToCenter;
  double CtCbis = CtC*cos(30.*3.14159/180.);
  int rings = PMarrays[ul].NumRings;
  bool XbyYarray = !PMarrays[ul].fUseRings;

  double x, y;
  QVector<APmPosAngTypeRecord> *pat = &PMarrays[ul].PositionsAnglesTypes;

  if (XbyYarray)
    { //X by Y type of array
      if (hexagonal)
        {
          for (int j=iY-1; j>-1; j--)
            for (int i=0; i<iX; i++)
              {
                x = CtC*( -0.5*(iX-1) +i  + 0.5*(j%2));
                y = CtCbis*(-0.5*(iY-1) +j);
                pat->append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
              }
        }
      else
        {
          for (int j=iY-1; j>-1; j--)
            for (int i=0; i<iX; i++)
              {
                x = CtC*(-0.5*(iX-1) +i);
                y = CtC*(-0.5*(iY-1) +j);
                pat->append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
              }
        }
    }
  else
    { //rings array
      if (hexagonal)
        {
          pat->append( APmPosAngTypeRecord(0,0,0, 0,0,0, PMarrays[ul].PMtype) );

          for (int i=1; i<rings+1; i++) //every new ring adds 6i PMs
            {
              x = i*CtC;
              y = 0;
              pat->append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
              for (int j=1; j<i+1; j++)
                {  //   //
                  x = pat->last().x - 0.5*CtC;
                  y = pat->last().y - CtCbis;
                  PMarrays[ul].PositionsAnglesTypes.append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
                }
              for (int j=1; j<i+1; j++)
                {   // --
                  x = pat->last().x - CtC;
                  y = pat->last().y;
                  PMarrays[ul].PositionsAnglesTypes.append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
                }
              for (int j=1; j<i+1; j++)
                {  // \\ //
                  x = pat->last().x - 0.5*CtC;
                  y = pat->last().y + CtCbis;
                  //PMs->insert(ipos, ul,x,y,z,Psi,typ);
                  PMarrays[ul].PositionsAnglesTypes.append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
                }
              for (int j=1; j<i+1; j++)
                {  // //
                  x = pat->last().x + 0.5*CtC;
                  y = pat->last().y + CtCbis;
                  PMarrays[ul].PositionsAnglesTypes.append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
                }
              for (int j=1; j<i+1; j++)
                {   // --
                  x = pat->last().x + CtC;
                  y = pat->last().y;
                  PMarrays[ul].PositionsAnglesTypes.append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
                }
              for (int j=1; j<i; j++)
                {  // \\       //dont do the last step - already positioned PM
                  x = pat->last().x + 0.5*CtC;
                  y = pat->last().y - CtCbis;
                  PMarrays[ul].PositionsAnglesTypes.append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
                }
            }
        }
      else {
          //using the same algorithm as X * Y
          iX = rings*2 + 1;
          iY = rings*2 + 1;
          for (int j=0; j<iY; j++)
            for (int i=0; i<iX; i++) {
                x = CtC*(-0.5*(iX-1) +i);
                y = CtC*(-0.5*(iY-1) +j);
                PMarrays[ul].PositionsAnglesTypes.append( APmPosAngTypeRecord(x,y,0, 0,0,0, PMarrays[ul].PMtype) );
              }
        }
    }
}

void DetectorClass::positionDummies()
{
  //creating all types - possible improvement - create only in use types of dummy pms
  int PMtypes = PMs->countPMtypes();
  QVector<TGeoVolume*> pmtDummy;
  pmtDummy.resize(PMtypes);
  for (int i=0; i<PMtypes; i++)
    {
      QString str = "dPM" + QString::number(i);
      QByteArray ba = str.toLocal8Bit();
      char *name = ba.data();
      const APmType *tp = PMs->getType(i);
      pmtDummy[i] = generatePmVolume(name, (*MpCollection)[tp->MaterialIndex]->GeoMed, tp);
      pmtDummy[i]->SetLineColor(30);
      pmtDummy[i]->SetTitle("p");
    }
  //positioning dummies
  for (int idum = 0; idum<PMdummies.size(); idum++)
    {
      const PMdummyStructure &dum = PMdummies.at(idum);
      bool hex = (PMs->getType(dum.PMtype)->Shape == 2);
      TGeoRotation *dummyPmRot = new TGeoRotation("dummypmRot", dum.Angle[0], dum.Angle[1], dum.Angle[2]+(hex ? 90.0 : 0.0));
      dummyPmRot->RegisterYourself();

      //check where is this node. Normal operation if it is World, other wise positioning it inside
      //the found volume
      TGeoNavigator* navi = GeoManager->GetCurrentNavigator();
      navi->SetCurrentPoint(dum.r[0], dum.r[1], dum.r[2]);
      TGeoNode* node = navi->FindNode();
      TGeoVolume* container = node->GetVolume();
      TString title = container->GetTitle();
      if (title != "-")
        {
          qWarning() << "Dummy PM cannot be placed inside a special volume (PM, dummy PM or optical grid)";
          ErrorString = "Attempt to place a dummy PM inside a forbidden container";
        }
      else if (container == top)
        {
          TGeoCombiTrans *dummyPmTrans = new TGeoCombiTrans("dummypmTrans", dum.r[0], dum.r[1], dum.r[2], dummyPmRot);
          top->AddNode(pmtDummy[dum.PMtype], idum, dummyPmTrans);
        }
      else
        {
//                  qDebug() << "dummyPM center detected to be NOT in the World container";
//                  qDebug() << "Detected node name:" << container->GetName();

          TGeoHMatrix* m = GeoManager->GetCurrentNavigator()->GetCurrentMatrix();
          TGeoHMatrix minv = m->Inverse();
          TGeoHMatrix final = minv * TGeoCombiTrans("combined", dum.r[0], dum.r[1], dum.r[2], dummyPmRot);

          TGeoHMatrix* final1 = new TGeoHMatrix(final);
          container->AddNode(pmtDummy[dum.PMtype], idum, final1);
        }
    }
}

void DetectorClass::updateWorldSize(double &XYm, double &Zm)
{
  Sandwich->World->updateWorldSize(XYm, Zm);

  //PMs
  for (int ipm=0; ipm<PMs->count(); ipm++)
    {
      double msize = 0.5*PMs->SizeZ(ipm);
      UpdateMax(msize, 0.5*PMs->SizeX(ipm));
      UpdateMax(msize, 0.5*PMs->SizeY(ipm));

      UpdateMax(XYm, fabs(PMs->X(ipm)) + msize);
      UpdateMax(XYm, fabs(PMs->Y(ipm)) + msize);
      UpdateMax(Zm,  fabs(PMs->Z(ipm)) + msize);
    }

  //DummyPMs
  for (int i=0; i<PMdummies.size(); i++)
    {
      double msize = 0.5*PMs->getType(PMdummies[i].PMtype)->SizeZ;
      UpdateMax(msize, 0.5*PMs->getType(PMdummies[i].PMtype)->SizeX);
      UpdateMax(msize, 0.5*PMs->getType(PMdummies[i].PMtype)->SizeY);

      UpdateMax(XYm, fabs(PMdummies[i].r[0]) + msize);
      UpdateMax(XYm, fabs(PMdummies[i].r[1]) + msize);
      UpdateMax(Zm,  fabs(PMdummies[i].r[2]) + msize);
  }
}

void DetectorClass::updatePreprocessingAddMultySize()
{
    //qDebug() << "On PM num change: fixing addmulti";
    QJsonObject js1 = Config->JSON["DetectorConfig"].toObject();
    if (!js1.contains("LoadExpDataConfig")) return;

    QJsonObject js = js1["LoadExpDataConfig"].toObject();
    if (js.isEmpty()) return;

    if (js.contains("AddMulti"))
    {
        QJsonArray ar = js["AddMulti"].toArray();
        if (ar.size() == PMs->count())
        {
            //fine, no need to update
            return;
        }
        else if (ar.size() < PMs->count())
        {
            while (ar.size() < PMs->count())
              {
                QJsonArray el;
                el << 0.0 << 1.0;
                ar << el;
              }
        }
        else
        {
            while (ar.size() > PMs->count())
                ar.removeLast();
        }

        js["AddMulti"] = ar;
        js1["LoadExpDataConfig"] = js;
        Config->JSON["DetectorConfig"] = js1;
    }
}

void removeOldFile(const QString & fileName, const QString & txt)
{
    QFile f(fileName);
    if (f.exists())
    {
        //qDebug() << "Removing old file with" << txt << ":" << fileName;
        bool bOK = f.remove();
        if (!bOK) qWarning() << "Was unable to remove old file with" << txt << ":" << fileName;
    }
}

#include "ag4simulationsettings.h"
bool DetectorClass::generateG4interfaceFiles(const AG4SimulationSettings & G4SimSet, int numThreads, bool bBuildTracks, bool bLogHistory, int maxTracks)
{
    QString gdmlName = G4SimSet.getGdmlFileName();
    QString err = exportToGDML(gdmlName);
    if ( !err.isEmpty() ) return false;

    QJsonObject json;

    json["PhysicsList"] = G4SimSet.PhysicsList;

    QJsonArray Parr;
    //const QStringList Particles = MpCollection->getListOfParticleNames();
    //for (auto & pname : Particles ) Parr << pname;
    const int numPart = MpCollection->countParticles();
    for (int iP=0; iP<numPart; iP++)
    {
        const AParticle * part = MpCollection->getParticle(iP);
        if (part->isIon())
        {
            QJsonArray ar;
            ar << part->ParticleName << part->ionZ << part->ionA;
            Parr << ar;
        }
        else Parr << part->ParticleName;
    }
    json["Particles"] = Parr;

    const QStringList Materials = MpCollection->getListOfMaterialNames();
    QJsonArray Marr;
    for (auto & mname : Materials ) Marr << mname;
    json["Materials"] = Marr;

    QJsonArray SVarr;
    for (auto & v : G4SimSet.SensitiveVolumes ) SVarr << v;
    json["SensitiveVolumes"] = SVarr;

    json["GDML"] = gdmlName;

    QJsonArray Carr;
    for (auto & c : G4SimSet.Commands ) Carr << c;
    json["Commands"] = Carr;

    json["GuiMode"] = false;

    json["LogHistory"] = bLogHistory;
    json["BuildTracks"] = bBuildTracks;
    if (bBuildTracks) json["MaxTracks"] = maxTracks;

    for (int i=0; i<numThreads; i++)
    {
        json["Seed"] = static_cast<int>(RandGen->Rndm()*10000000);

        QString primFN = G4SimSet.getPrimariesFileName(i);
        json["File_Primaries"] = primFN;
        removeOldFile(primFN, "primaries");

        QString depoFN = G4SimSet.getDepositionFileName(i);
        json["File_Deposition"] = depoFN;
        removeOldFile(depoFN, "deposition");

        QString recFN = G4SimSet.getReceitFileName(i);
        json["File_Receipt"] = recFN;
        removeOldFile(recFN, "receipt");

        QString tracFN = G4SimSet.getTracksFileName(i);
        json["File_Tracks"] = tracFN;
        removeOldFile(tracFN, "tracking");

        SaveJsonToFile(json, G4SimSet.getConfigFileName(i));
    }

    return true;
}
