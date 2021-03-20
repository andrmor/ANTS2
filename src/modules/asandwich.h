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
class TGeoCombiTrans;

class ASandwich : public QObject
{  
  Q_OBJECT
public:
  enum SlabState {CommonShapeSize = 0, CommonShape, Individual};

  ASandwich();
  ~ASandwich();

  AGeoObject * World      = nullptr;  // world tree structure, slabs are on the first level!
  AGeoObject * Prototypes = nullptr;  // hosts prototypes; a prototype is an objects (can be a tree of objects) which can be placed multiple times in the geometry
                                      // Prorotypes object is hosted by World!

  void         clearWorld();
  bool         canBeDeleted(AGeoObject * obj) const;

  //slab handling
  void         appendSlab(ASlabModel * slab);
  void         insertSlab(int index, ASlabModel * slab);
  int          countSlabs() const;
  AGeoObject * findSlabByIndex(int index);
  void         enforceCommonProperties();

  //lightguide handling
  AGeoObject * getUpperLightguide();  // 0 if not defined
  AGeoObject * getLowerLightguide();  // 0 if not defined
  AGeoObject * addLightguide(bool upper); // new object (lightguide) is added to the WorldTree
  bool         convertObjToLightguide(AGeoObject * obj, bool upper);

  //composite
  void         convertObjToComposite(AGeoObject * obj);

  //ptototype/instances
  QString      convertToNewPrototype(QVector<AGeoObject*> members);
  bool         isValidPrototypeName(const QString & ProtoName) const;

  //grid
  void         convertObjToGrid(AGeoObject * obj);
  void         shapeGrid(AGeoObject * obj, int shape, double p0, double p1, double p2, int wireMat);
  //parallel - 0, pitch, length, wireDiameter
  //mesh - 1, pitchX, pitchY, wireDiameter
  //hexa - 2, outer circle diameter, inner circle diameter, full height

  void         populateGeoManager(TGeoVolume * top, TGeoManager * geoManager, AMaterialParticleCollection * materialCollection, QVector<APMandDummy> * vPMsAndDumPMs);

  void         UpdateDetector(); //trigger this to update the detector
  void         ChangeState(ASandwich::SlabState State); //triggered by GUI
  bool         CalculateZofSlabs();
  QStringList  GetMaterials() const {return Materials;}
  bool         isMaterialsEmpty() {return Materials.isEmpty();}
  bool         isMaterialInUse(int imat) const;
  void         DeleteMaterial(int imat);
  bool         isVolumeExistAndActive(const QString & name) const;
  void         changeLineWidthOfVolumes(int delta);

  // JSON
  void         writeToJson(QJsonObject & json);
  QString      readFromJson(QJsonObject & json);  // returns "" if no errors, else error description

  // for particle remove - handled by AConfiguration!
  void         IsParticleInUse(int particleId, bool & bInUse, QString & MonitorNames) const;
  void         RemoveParticle(int particleId); //should NOT be used to remove one of particles in use! use onIsPareticleInUse first

  //World size-related
  bool         isWorldSizeFixed() const;
  void         setWorldSizeFixed(bool bFlag);
  double       getWorldSizeXY() const;
  void         setWorldSizeXY(double size);
  double       getWorldSizeZ() const;
  void         setWorldSizeZ(double size);

  SlabState SandwichState = CommonShapeSize;
  QStringList Materials;  // list of currently defined materials
  ASlabXYModel * DefaultXY = nullptr;
  int ZOriginType = 0; //-1 top, 0 mid, 1 bottom (of the slab with fCenter = true)
  QVector<AGridElementRecord*> GridRecords;

  QVector<const AGeoObject*> MonitorsRecords;
  QVector<QString> MonitorIdNames; //runtime
  QVector<TGeoNode *> MonitorNodes; //runtime

  double Z_UpperBound, Z_LowerBound;   // slab Z bounds, available after CalculateZofSlabs()

  QString LastError;  

  //temporary, used during the call of populateGeoManager()
  TGeoManager * GeoManager = nullptr;
  AMaterialParticleCollection * MaterialCollection = nullptr;
  QVector<APMandDummy> * PMsAndDumPMs = nullptr;

signals:
  void RequestGuiUpdate();
  void RequestRebuildDetector();
  void WarningMessage(QString);

public slots:
  void onMaterialsChanged(const QStringList MaterialList);  // !*! obsolete?  //October2020: disabled signal emitting to request gui update

private:
  void clearModel();
  void clearGridRecords();
  void clearMonitorRecords();

  void importFromOldStandardJson(QJsonObject& json, bool fPrScintCont);
  void importOldLightguide(QJsonObject& json, bool upper);
  void importOldMask(QJsonObject &json);
  void importOldGrid(QJsonObject &json);

  void addTGeoVolumeRecursively(AGeoObject * obj, TGeoVolume * parent, int forcedNodeNumber = 0);

  void positionArrayElement(int ix, int iy, int iz, AGeoObject * el, AGeoObject * arrayObj, TGeoVolume * parent, int arrayIndex);
  void positionCircularArrayElement(int ia, AGeoObject * el, AGeoObject * arrayObj, TGeoVolume * parent, int arrayIndex);
  void positionStackObject(AGeoObject * obj, const AGeoObject * RefObj, TGeoVolume * parent, int forcedNodeNumber);
  void positionArrayElement_StackObject(int ix, int iy, int iz, AGeoObject *obj, const AGeoObject *RefObj, AGeoObject *arrayObj, TGeoVolume *parent, int arrayIndex);

  void expandPrototypeInstances();
  bool processCompositeObject(AGeoObject *obj);
  void positionHostedForLightguide(AGeoObject *obj, TGeoVolume *vol, TGeoCombiTrans *lTrans);
  void addMonitorNode(AGeoObject *obj, TGeoVolume *vol, TGeoVolume *parent, TGeoCombiTrans *lTrans);
};

#endif // ASANDWICH_H
