#ifndef AGEOOBJECT_H
#define AGEOOBJECT_H

#include "amonitorconfig.h"

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>

class QJsonObject;
class ATypeGeoObject;
class TGeoShape;
class AGeoShape;
class ASlabModel;
class AGridElementRecord;

class AGeoObject
{
public:
  AGeoObject(QString name = "", QString ShapeGenerationString = ""); //name must be unique!
  AGeoObject(const AGeoObject * objToCopy);  //normal object is created
  AGeoObject(const QString & name, const QString & container, int iMat, AGeoShape * shape,
             double x, double y, double z,   double phi, double theta, double psi); // for tmp only, needs positioning in the container and name uniqueness check!
  AGeoObject(ATypeGeoObject * objType, AGeoShape * shape = nullptr);  // pointers are assigned to the object properties! if no shape, default cube is formed
  ~AGeoObject();

  bool readShapeFromString(const QString & GenerationString, bool OnlyCheck = false); // using parameter values taken from gui generation string
  void DeleteMaterialIndex(int imat);
  void makeItWorld();
  bool isWorld() const;
  int  getMaterial() const;

  const AGeoObject * isGeoConstInUse(const QRegExp & nameRegExp) const;
  void replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName);
  const AGeoObject * isGeoConstInUseRecursive(const QRegExp & nameRegExp) const;
  void replaceGeoConstNameRecursive(const QRegExp & nameRegExp, const QString & newName);

  //json for a single object
  void writeToJson(QJsonObject & json);
  void readFromJson(const QJsonObject & json);

  //recursive json, using single object json
  void writeAllToJarr(QJsonArray & jarr);
  QString readAllFromJarr(AGeoObject * World, const QJsonArray & jarr);  // returns "" if no errors

  ATypeGeoObject * ObjectType = nullptr;  // always created in the constructor!
  AGeoShape * Shape = nullptr;            // allowed to remain nullptr after construction!

  QString Name;
  int Material = 0;
  double Position[3];
  double Orientation[3];
  bool fLocked = false;
  bool fActive = true;

  QString PositionStr[3];
  QString OrientationStr[3];

  AGeoObject * Container = nullptr;
  QList<AGeoObject*> HostedObjects;

  //visualization properties
  int color = 1; // initializaed as -1, updated when first time shown by SlabListWidget
  int style = 1;
  int width = 1;

  QString LastScript;

  //for slabs
  bool UpdateFromSlabModel(ASlabModel * SlabModel);
  ASlabModel* getSlabModel();       // danger! it returns 0 if the object is not slab!
  void setSlabModel(ASlabModel* slab);  // force-converts to slab type!

  //for composite
  AGeoObject* getContainerWithLogical();
  const AGeoObject* getContainerWithLogical() const;
  bool isCompositeMemeber() const;
  void refreshShapeCompositeMembers(AGeoShape * ExternalShape = nullptr); //safe to use on any AGeoObject; if ExternalShape is provided , it is updated; otherwise, Objects's shape is updated
  bool isInUseByComposite(); //safe to use on any AGeoObject
  void clearCompositeMembers();
  void removeCompositeStructure();
  void updateNameOfLogicalMember(const QString & oldName, const QString & newName);

  //for grid
  AGeoObject* getGridElement();
  void  updateGridElementShape();
  AGridElementRecord* createGridRecord();

  //for monitor
  void updateMonitorShape();
  const AMonitorConfig * getMonitorConfig() const; //returns nullptr if obj is not a monitor

  //for stacks
  bool isStackMember() const;
  bool isStackReference() const;
  AGeoObject * getOrMakeStackReferenceVolume();  // for stack container or members
  void updateStack();  //called on one object of the set - it is used to calculate positions of other members!
  void updateAllStacks();

  // the following checks are always done DOWN the chain
  // for global effect, the check has to be performed on World (Top) object
  AGeoObject * findObjectByName(const QString & name);
  void findObjectsByWildcard(const QString & name, QVector<AGeoObject*> & foundObjs);
  void changeLineWidthRecursive(int delta);
  bool isNameExists(const QString & name);
  bool isContainsLocked();
  bool isDisabled() const;
  void enableUp();
  bool isFirstSlab(); //slab or lightguide
  bool isLastSlab();  //slab or lightguide
  bool containsUpperLightGuide();
  bool containsLowerLightGuide(); 
  AGeoObject* getFirstSlab();
  AGeoObject* getLastSlab();
  int getIndexOfFirstSlab();
  int getIndexOfLastSlab();
  void addObjectFirst(AGeoObject* Object);
  void addObjectLast(AGeoObject* Object);  //before slabs!
  bool migrateTo(AGeoObject* objTo, bool fAfter = false, AGeoObject *reorderObj = nullptr);
  bool repositionInHosted(AGeoObject* objTo, bool fAfter);
  bool suicide(bool SlabsToo = false); // not possible for locked and static objects
  void recursiveSuicide(); // does not remove locked and static objects, but removes all unlocked objects down the chain
  void lockUpTheChain();
  void lockBuddies();
  void lockRecursively();
  void unlockAllInside();
  void clearAll();    // bad name!
  void clearContent();
  void updateWorldSize(double& XYm, double& Zm);
  bool isMaterialInUse(int imat) const;  //including disabled objects
  bool isMaterialInActiveUse(int imat) const;  //excluding disabled objects
  void collectContainingObjects(QVector<AGeoObject*> & vec) const;
  double getMaxSize() const;

  bool getPositionInWorld(double * worldPos) const;

  bool isContainerValidForDrop(QString &errorStr) const;

  AGeoObject * makeClone(AGeoObject * World); // returns nullptr if failed; garantees unique names if World is not nullptr; Slabs are not properly cloned while there is a special container with them!
  AGeoObject * makeCloneForInstance(const QString & suffix);

  void findAllInstancesRecursive(QVector<AGeoObject*> & Instances);

  //service propertie
  QString tmpContName;   //used only during load
  bool fExpanded = true; //gui only - is this object expanded in tree view

private:
  AGeoObject * findContainerUp(const QString & name);

  void constructorInit();

  void enforceUniqueNameForCloneRecursive(AGeoObject * World, AGeoObject & tmpContainer);
  void addSuffixToNameRecursive(const QString & suffix);

public:
  static QString GenerateRandomName();
  static QString GenerateRandomObjectName();
  static QString GenerateRandomLightguideName();
  static QString GenerateRandomCompositeName();
  static QString GenerateRandomArrayName();
  static QString GenerateRandomGridName();
  static QString GenerateRandomMaskName();
  static QString GenerateRandomGuideName();
  static QString GenerateRandomGroupName();
  static QString GenerateRandomStackName();
  static QString GenerateRandomMonitorName();

  static QString generateCloneObjName(const QString & initialName);

};

#endif // AGEOOBJECT_H
