#ifndef DETECTORCLASS_H
#define DETECTORCLASS_H

#include "apmarraydata.h"

#include <QVector>
#include <QObject>
#include <QJsonObject>

#include "TMathBase.h"

class AParticle;
class TGeoManager;
class ALrfModuleSelector;
class APmHub;
class AMaterialParticleCollection;
class TGeoVolume;
class TGeoMedium;
class TRandom2;
class AConfiguration;
class APreprocessingSettings;
class ASandwich;
class APmGroupsManager;
class APmType;

struct PMdummyStructure
{
  int PMtype; //index of the PM type
  int UpperLower; //Belongs to a PM plane: 0- upper, 1 - lower
  double r[3]; //PMs center coordinates
  double Angle[3]; //Phi, Theta, Psi;

  void writeToJson(QJsonObject &json);
  bool readFromJson(QJsonObject &json);
};

//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

class DetectorClass : public QObject
{
  Q_OBJECT

public:
  AConfiguration* Config;              // contains json config file
  TGeoManager* GeoManager;             // detector geometry, navigator and visualizator
  APmHub* PMs;                            // all info on PMs 
  ALrfModuleSelector* LRFs;            // detector response
  APmGroupsManager* PMgroups;          // all info on defined PM groups, including rec settings jsons
  TGeoVolume* top;                     // world in GeoManager
  TRandom2* RandGen;                   // random generator

  QVector<APmArrayData> PMarrays;  //upper(0) and lower(1) PM array data
  QVector<PMdummyStructure> PMdummies; // all info on PM dummies
  QString AddObjPositioningScript;
  ASandwich* Sandwich;

  AMaterialParticleCollection* MpCollection; // collection of defined materials and particles  

  QJsonObject PreprocessingJson;

  bool fSecScintPresent;

  double WorldSizeXY, WorldSizeZ; //size of World volume
  bool fWorldSizeFixed;  //fixed and defined by GUI

  QString ErrorString;

  DetectorClass(AConfiguration* config);
  ~DetectorClass();

  bool MakeDetectorFromJson(QJsonObject &json);
  bool BuildDetector(bool SkipSimGuiUpdate = false, bool bSkipAllUpdates = false);   // build detector from JSON //on load config, set SkipSimGuiUpdate = true since json is still old!
  bool BuildDetector_CallFromScript(); // save current detector to JSON, then call BuildDetector()

  void writeToJson(QJsonObject &json);

  bool makeSandwichDetector();
  bool importGDML(QString gdml);
  bool isGDMLempty() const {return GDML.isEmpty();}
  void clearGDML() {GDML = "";}
  int checkGeoOverlaps();   // checks for overlaps in the geometry (GeoManager) and returns the number of overlaps
  void checkSecScintPresent();
  void colorVolumes(int scheme, int id = 0);
  int pmCount() const; // Raimundo: so it's not necessary to #include<pms.h> just to clear LRFs
  void findPM(int ipm, int &ul, int &index);
  const QString removePMtype(int itype);

  //write to Json - can be used from outside
  void writeWorldFixedToJson(QJsonObject &json);
  void writePMarraysToJson(QJsonObject &json);
  void writeDummyPMsToJson(QJsonObject &json);  
  void writeGDMLtoJson(QJsonObject &json);
  void writePreprocessingToJson(QJsonObject &json);

  void changeLineWidthOfVolumes(int delta);

  const QString exportToGDML(const QString &fileName) const; //returns error string, empty if all is fine  
  const QString exportToROOT(const QString& fileName) const;
  
  void saveCurrentConfig(const QString &fileName);

public slots:
  void onRequestRegisterGeoManager();

private:
  //reads
  void readWorldFixedFromJson(QJsonObject &json);
  bool readPMarraysFromJson(QJsonObject &json);
  bool readDummyPMsFromJson(QJsonObject &json);

  void constructDetector();

  QString GDML;
  bool processGDML(); //check validity, discard if bad and return to sandwich  

  Double_t UpperEdge, LowerEdge; //used to calculate Z positions of detector elements
  TGeoVolume *generatePmVolume(TString Name, TGeoMedium *Medium, const APmType *tp);
  void populatePMs();
  void positionPMs();
  void calculatePmsXY(int ul);
  void positionDummies();
  void updateWorldSize(double &XYm, double &Zm);
  void updatePreprocessingAddMultySize();

signals:
  void ColorSchemeChanged(int scheme, int matId); //in case GUI wants to update coloring
                                                  //0-normal, 1-by mat, 2-highlight mat (matId)
  void requestClearEventsData();
  void requestGroupsGuiUpdate();
  void newGeoManager();
};

#endif // DETECTORCLASS_H
