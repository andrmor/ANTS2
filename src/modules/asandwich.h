#ifndef ASANDWICH_H
#define ASANDWICH_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include "apmanddummy.h"

class ASlabModel;
class ASlabXYModel;
class AGeoObject;
class TGeoManager;
class TGeoVolume;
class AMaterialParticleCollection;
class AGridElementRecord;
class TGeoNode;

class ASandwich : public QObject
{  
  Q_OBJECT
public:
  enum SlabState {CommonShapeSize = 0, CommonShape, Individual};

  ASandwich();
  ~ASandwich();

  AGeoObject * World = nullptr;     //world with tree structure, slabs are on the first level!
  void clearWorld();

  //slab handling
  void appendSlab(ASlabModel* slab);
  void insertSlab(int index, ASlabModel* slab);
  int countSlabs();
  AGeoObject* findSlabByIndex(int index);
  void enforceCommonProperties();

  //lightguide handling
  AGeoObject* getUpperLightguide();  // 0 if not defined
  AGeoObject* getLowerLightguide();  // 0 if not defined
  AGeoObject *addLightguide(bool upper); // new object (lightguide) is added to the WorldTree
  bool convertObjToLightguide(AGeoObject *obj, bool upper);

  //composite
  void convertObjToComposite(AGeoObject *obj);

  //grid
  void convertObjToGrid(AGeoObject *obj);
  void shapeGrid(AGeoObject* obj, int shape, double p0, double p1, double p2, int wireMat);
  //parallel - 0, pitch, length, wireDiameter
  //mesh - 1, pitchX, pitchY, wireDiameter
  //hexa - 2, outer circle diameter, inner circle diameter, full height

  // populate TGeoManager
  void addTGeoVolumeRecursively(AGeoObject* obj, TGeoVolume* parent,
                                TGeoManager* GeoManager, AMaterialParticleCollection* MaterialCollection,
                                QVector<APMandDummy> *PMsAndDumPMs,
                                int forcedNodeNumber = 0);

  void clearGridRecords();
  void clearMonitorRecords();

  void UpdateDetector(); //trigger this to update the detector
  void ChangeState(ASandwich::SlabState State); //triggered by GUI
  bool CalculateZofSlabs();
  QStringList GetMaterials() const {return Materials;}   // to const QStringList &
  bool isMaterialsEmpty() {return Materials.isEmpty();}
  bool isMaterialInUse(int imat);
  void DeleteMaterial(int imat);
  bool isVolumeExist(QString name);
  void changeLineWidthOfVolumes(int delta);

  // JSON
  void writeToJson(QJsonObject& json);
  void readFromJson(QJsonObject& json);

  // for particle remove - handled by AConfiguration!
  void IsParticleInUse(int particleId, bool &bInUse, QString& MonitorNames);
  void RemoveParticle(int particleId); //should NOT be used to remove one of particles in use! use onIsPareticleInUse first

  //World size-related
  bool   isWorldSizeFixed() const;
  void   setWorldSizeFixed(bool bFlag);
  double getWorldSizeXY() const;
  void   setWorldSizeXY(double size);
  double getWorldSizeZ() const;
  void   setWorldSizeZ(double size);

  ASandwich::SlabState SandwichState;
  QStringList Materials;  // list of currently defined materials
  ASlabXYModel* DefaultXY;
  int ZOriginType; //-1 top, 0 mid, 1 bottom (of the slab with fCenter = true)
  QVector<AGridElementRecord*> GridRecords;

  // pointers to monitors
  QVector<const AGeoObject*> MonitorsRecords;
  QVector<QString> MonitorIdNames; //runtime
  QVector<TGeoNode *> MonitorNodes; //runtime

  // available after calculation of Z of layers
  double Z_UpperBound, Z_LowerBound;

  QString LastError;  

signals:
  void RequestGuiUpdate();       //does NOT trigger remake detector in GUI mode
  void RequestRebuildDetector(); //triggers remake detector in GUI mode
  void WarningMessage(QString);

public slots:
  void onMaterialsChanged(const QStringList MaterialList);

private:
  void clearModel();
  void importFromOldStandardJson(QJsonObject& json, bool fPrScintCont);
  void importOldLightguide(QJsonObject& json, bool upper);
  void importOldMask(QJsonObject &json);
  void importOldGrid(QJsonObject &json);
  void positionArrayElement(int ix, int iy, int iz,
                            AGeoObject *el, AGeoObject *arrayObj, TGeoVolume *parent,
                            TGeoManager *GeoManager, AMaterialParticleCollection *MaterialCollection, QVector<APMandDummy> *PMsAndDumPMs,
                            int arrayIndex = 0);
};

#endif // ASANDWICH_H
