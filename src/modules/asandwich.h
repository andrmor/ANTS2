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

  AGeoObject * World = nullptr;       // world tree structure, slabs are on the first level!
  AGeoObject * Prototypes = nullptr;  // hosts prototypes; a prototype is an objects (can be a tree of objects) which can be placed multiple times in the geometry

  void clearWorld();
  bool canBeDeleted(AGeoObject * obj) const;

  //slab handling
  void appendSlab(ASlabModel * slab);
  void insertSlab(int index, ASlabModel * slab);
  int  countSlabs();
  AGeoObject* findSlabByIndex(int index);
  void enforceCommonProperties();

  //lightguide handling
  AGeoObject * getUpperLightguide();  // 0 if not defined
  AGeoObject * getLowerLightguide();  // 0 if not defined
  AGeoObject * addLightguide(bool upper); // new object (lightguide) is added to the WorldTree
  bool convertObjToLightguide(AGeoObject * obj, bool upper);

  //composite
  void convertObjToComposite(AGeoObject * obj);

  //ptototype/instances
  QString convertToNewPrototype(QVector<AGeoObject*> members);
  bool isValidPrototypeName(const QString & ProtoName) const;

  //grid
  void convertObjToGrid(AGeoObject * obj);
  void shapeGrid(AGeoObject * obj, int shape, double p0, double p1, double p2, int wireMat);
  //parallel - 0, pitch, length, wireDiameter
  //mesh - 1, pitchX, pitchY, wireDiameter
  //hexa - 2, outer circle diameter, inner circle diameter, full height

  // populate TGeoManager
  void expandPrototypeInstances();
  void addTGeoVolumeRecursively(AGeoObject * obj, TGeoVolume * parent,
                                TGeoManager * GeoManager, AMaterialParticleCollection * MaterialCollection,
                                QVector<APMandDummy> * PMsAndDumPMs,
                                int forcedNodeNumber = 0);

  void clearGridRecords();
  void clearMonitorRecords();

  void UpdateDetector(); //trigger this to update the detector
  void ChangeState(ASandwich::SlabState State); //triggered by GUI
  bool CalculateZofSlabs();
  QStringList GetMaterials() const {return Materials;}
  bool isMaterialsEmpty() {return Materials.isEmpty();}
  bool isMaterialInUse(int imat);
  void DeleteMaterial(int imat);
  bool isVolumeExistAndActive(const QString & name) const;
  void changeLineWidthOfVolumes(int delta);

  // JSON
  void writeToJson(QJsonObject & json);
  QString readFromJson(QJsonObject & json);  // returnd "" if no errors, else error description

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

  ASandwich::SlabState SandwichState = CommonShapeSize;
  QStringList Materials;  // list of currently defined materials
  ASlabXYModel * DefaultXY = nullptr;
  int ZOriginType = 0; //-1 top, 0 mid, 1 bottom (of the slab with fCenter = true)
  QVector<AGridElementRecord*> GridRecords;

  // pointers to monitors
  QVector<const AGeoObject*> MonitorsRecords;
  QVector<QString> MonitorIdNames; //runtime
  QVector<TGeoNode *> MonitorNodes; //runtime

  // available after calculation of Z of layers
  double Z_UpperBound, Z_LowerBound;

  QString LastError;  

signals:
  void RequestGuiUpdate();
  void RequestRebuildDetector();
  void WarningMessage(QString);

public slots:
  void onMaterialsChanged(const QStringList MaterialList);  // !*! obsolete?  //October2020: disabled signal emitting to request gui update

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
