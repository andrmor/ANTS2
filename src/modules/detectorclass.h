#ifndef DETECTORCLASS_H
#define DETECTORCLASS_H

#include "apmarraydata.h"
#include "apmdummystructure.h"

#include <QObject>
#include <QVector>
#include <QJsonObject>

#include "TString.h"

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

class DetectorClass : public QObject
{
  Q_OBJECT

public:
  AConfiguration     * Config       = nullptr;
  TRandom2           * RandGen      = nullptr;
  AMaterialParticleCollection* MpCollection = nullptr;
  ASandwich          * Sandwich     = nullptr;
  APmHub             * PMs          = nullptr;
  ALrfModuleSelector * LRFs         = nullptr;
  APmGroupsManager   * PMgroups     = nullptr;
  TGeoManager        * GeoManager   = nullptr;
  TGeoVolume         * top          = nullptr;         // world in GeoManager

  QVector<APmArrayData>      PMarrays;                 // upper(0) and lower(1) PM array data
  QVector<APMdummyStructure> PMdummies;
  QString                    AddObjPositioningScript;

  QJsonObject PreprocessingJson;

  bool fSecScintPresent = false;

  QString ErrorString;

  DetectorClass(AConfiguration* config);
  ~DetectorClass();

  bool BuildDetector(bool SkipSimGuiUpdate = false, bool bSkipAllUpdates = false);   // build detector from JSON //on load config, set SkipSimGuiUpdate = true since json is still old!
  bool BuildDetector_CallFromScript(); // save current detector to JSON, then call BuildDetector()

  void writeToJson(QJsonObject &json);

  bool makeSandwichDetector();
  bool importGDML(const QString & gdml);
  bool isGDMLempty() const;
  void clearGDML();
  int  checkGeoOverlaps();   // checks for overlaps in the geometry (GeoManager) and returns the number of overlaps
  void checkSecScintPresent();
  void colorVolumes(int scheme, int id = 0);  // !*! can be very slow for large detectors!
  int  pmCount() const;
  void findPM(int ipm, int &ul, int &index);
  QString removePMtype(int itype);
  void assignSaveOnExitFlag(const QString & VolumeName);
  void clearTracks();
  void assureNavigatorPresent();

  //write to Json - can be used from outside
  //void writeWorldFixedToJson(QJsonObject &json);
  void writePMarraysToJson(QJsonObject &json);
  void writeDummyPMsToJson(QJsonObject &json);  
  void writeGDMLtoJson(QJsonObject &json);
  void writePreprocessingToJson(QJsonObject &json);

  void changeLineWidthOfVolumes(int delta);

  const QString exportToGDML(const QString & fileName) const; //returns error string, empty if all is fine
  const QString exportToROOT(const QString & fileName) const;
  
  void saveCurrentConfig(const QString &fileName);

public slots:
  void onRequestRegisterGeoManager();

private:
  //reads
  bool readWorldFixedFromJson(const QJsonObject & json);
  bool readPMarraysFromJson(QJsonObject &json);
  bool readDummyPMsFromJson(QJsonObject &json);

  void populateGeoManager();

  QString GDML;
  bool processGDML(); //check validity, discard if bad and return to sandwich  

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
