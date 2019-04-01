#ifndef AGEOOBJECT_H
#define AGEOOBJECT_H

#include "amonitorconfig.h"

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QVector>

class ATypeObject;
class TGeoShape;
class AGeoShape;
class ASlabModel;
class AGridElementRecord;

class AGeoObject
{
public:
  AGeoObject(QString name = "", QString ShapeGenerationString = ""); //name must be unique!
  AGeoObject(const AGeoObject* objToCopy);  //normal object is created
  AGeoObject(QString name, QString container, int iMat, AGeoShape *shape,
             double x, double y, double z,   double phi, double theta, double psi); // for tmp only, needs positioning in the container and name uniqueness check!
  AGeoObject(ATypeObject *objType, AGeoShape* shape = 0);  // pointers are assigned to the object properties! if no shape, default cube is formed
  ~AGeoObject();

  bool readShapeFromString(QString GenerationString, bool OnlyCheck = false); // using parameter values taken from gui generation string
  void DeleteMaterialIndex(int imat);
  void makeItWorld();

  //json for a single object
  void writeToJson(QJsonObject& json);
  void readFromJson(QJsonObject& json);

  //recursive json, using single object json
  void writeAllToJarr(QJsonArray& jarr);
  void readAllFromJarr(AGeoObject *World, QJsonArray& jarr);

  ATypeObject* ObjectType;
  AGeoShape* Shape;

  QString Name;
  int Material;
  double Position[3];
  double Orientation[3];
  bool fLocked;
  bool fActive;

  AGeoObject* Container;
  QList<AGeoObject*> HostedObjects;

  //visualization properties
  int color; // initializaed as -1, updated when first time shown by SlabListWidget
  int style;
  int width;

  QString LastScript;

  //for slabs
  void UpdateFromSlabModel(ASlabModel* SlabModel);
  ASlabModel* getSlabModel();       // danger! it returns 0 if the object is not slab!
  void setSlabModel(ASlabModel* slab);  // force-converts to slab type!

  //for composite
  AGeoObject* getContainerWithLogical();
  bool isCompositeMemeber() const;
  void refreshShapeCompositeMembers(AGeoShape* ExternalShape = 0); //safe to use on any AGeoObject; if ExternalShape is provided , it is updated; otherwise, Objects's shape is updated
  bool isInUseByComposite(); //safe to use on any AGeoObject
  void clearCompositeMembers();

  //for grid
  AGeoObject* getGridElement();
  void  updateGridElementShape();
  AGridElementRecord* createGridRecord();

  //for monitor
  void updateMonitorShape();

  // the following checks are always done DOWN the chain
  // for global effect, the check has to be performed on World (Top) object
  AGeoObject* findObjectByName(const QString name);
  void changeLineWidthRecursive(int delta);
  bool isNameExists(const QString name);
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
  bool migrateTo(AGeoObject* objTo);
  bool repositionInHosted(AGeoObject* objTo, bool fAfter);
  bool suicide(bool SlabsToo = false); // not possible for locked and static objects
  void recursiveSuicide(); // does not remove locked and static objects, but removes all unlocked objects down the chain
  void lockUpTheChain();
  void lockBuddies();
  void lockRecursively();
  void unlockAllInside();
  void updateStack();  //called on one object of the set - it is used to calculate positions of other members!
  void clearAll();
  void updateWorldSize(double& XYm, double& Zm);
  bool isMaterialInUse(int imat) const;  //including disabled objects
  bool isMaterialInActiveUse(int imat) const;  //excluding disabled objects

  //service propertie
  QString tmpContName; //used only during load:
  bool fExpanded; //gui only - is this object expanded in tree view

private:
  AGeoObject* findContainerUp(const QString name);  
  void constructorInit();

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

  static bool CheckPointsForArb8(QList<QPair<double, double> > V );

  static AGeoShape* GeoShapeFactory(const QString ShapeType);  // SHAPE FACTORY !!!
  static QList<AGeoShape*> GetAvailableShapes();               // list of available shapes for generation of help and highlighter: do not forget to add new here!

};

// ============== Object type ==============

class ATypeObject
{  
public:
    virtual ~ATypeObject() {}

    bool isHandlingStatic() const   {return Handling == "Static";}
    bool isHandlingStandard() const {return Handling == "Standard";}
    bool isHandlingSet() const      {return Handling == "Set";}
    bool isHandlingArray() const    {return Handling == "Array";}

    bool isWorld() const            {return Type == "World";}
    bool isSlab() const             {return Type == "Slab" || Type == "Lightguide";}  //lightguide is also Slab!
    bool isLightguide() const       {return Type == "Lightguide";}  //lightguide is also Slab!
    bool isUpperLightguide() const;
    bool isLowerLightguide() const;
    bool isGroup() const            {return Type == "Group";}
    bool isStack() const            {return Type == "Stack";}
    bool isLogical() const          {return Type == "Logical";}
    bool isSingle() const           {return Type == "Single";}
    bool isCompositeContainer() const {return Type == "CompositeContainer";}
    bool isComposite() const        {return Type == "Composite";}
    bool isArray() const            {return Type == "Array";}
    bool isGrid() const             {return Type == "Grid";}
    bool isGridElement() const      {return Type == "GridElement";}
    bool isMonitor() const          {return Type == "Monitor";}

    virtual void writeToJson(QJsonObject& json) { json["Type"] = Type; } // virtual: CALL THIS, then save additional properties the concrete type has
    virtual void readFromJson(QJsonObject& json) = 0;  // virtual: read additional properties the concrete type has

    static ATypeObject* TypeObjectFactory(const QString Type);  // TYPE FACTORY !!!

protected:
    QString Type;
    QString Handling;
};

  //static objects
class ATypeWorldObject : public ATypeObject
{
public:
    ATypeWorldObject() {Type = "World"; Handling = "Static";}

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeSlabObject : public ATypeObject
{
public:
    ATypeSlabObject(); //Slab Static
    ~ATypeSlabObject();

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); } //no need to save slab, it will be assigned from outside
    virtual void readFromJson(QJsonObject& ) {}

    ASlabModel* SlabModel;
};
class ATypeLightguideObject : public ATypeSlabObject  //inherits from Slab!
{
public:
    ATypeLightguideObject(); //Lightguide Static

    enum UpperLowerEnum {Upper = 0, Lower};

    virtual void writeToJson(QJsonObject& json); //no need to save slab, it will be assigned from outside
    virtual void readFromJson(QJsonObject& json);

    UpperLowerEnum UpperLower;
};


  // Set objects
class ATypeGroupContainerObject : public ATypeObject
{
public:
    ATypeGroupContainerObject() {Type = "Group"; Handling = "Set";}

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeStackContainerObject : public ATypeObject
{
public:
    ATypeStackContainerObject() {Type = "Stack"; Handling = "Set";}

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeCompositeContainerObject : public ATypeObject
{
public:
    ATypeCompositeContainerObject() {Type = "CompositeContainer"; Handling = "Set";}

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};

  // AddObjects - objects which are added to geometry using the standard procedure
class ATypeSingleObject : public ATypeObject
{
public:
    ATypeSingleObject() {Type = "Single"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeCompositeObject : public ATypeObject
{
public:
    ATypeCompositeObject() {Type = "Composite"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};

class ATypeArrayObject : public ATypeObject
{
public:
    ATypeArrayObject() {Type = "Array"; Handling = "Array"; numX = 2; numY = 2; numZ = 1; stepX = 25; stepY = 25; stepZ = 25;}
    ATypeArrayObject(int numX, int numY, int numZ, double stepX, double stepY, double stepZ)
        : numX(numX), numY(numY), numZ(numZ), stepX(stepX), stepY(stepY), stepZ(stepZ) {Type = "Array"; Handling = "Array";}

    void Reconfigure(int NumX, int NumY, int NumZ, double StepX, double StepY, double StepZ)
    {
        numX = NumX; numY = NumY; numZ = NumZ;
        stepX = StepX; stepY = StepY; stepZ = StepZ;
    }

    virtual void writeToJson(QJsonObject& json);
    virtual void readFromJson(QJsonObject& json);

    int numX, numY, numZ;
    double stepX, stepY, stepZ;
};

class ATypeGridObject : public ATypeObject
{
public:
    ATypeGridObject() {Type = "Grid"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json) { ATypeObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeGridElementObject : public ATypeObject
{
public:
    ATypeGridElementObject() {Type = "GridElement"; Handling = "Standard"; shape = 1; size1 = 10; size2 = 10; dz = 5;}

    virtual void writeToJson(QJsonObject& json);
    virtual void readFromJson(QJsonObject& json);

    int shape; //0 : rectanglar-2wires, 1 : rectangular-crossed,  2 : hexagonal
    double size1, size2; //half sizes for rectangular, size1 is size of hexagon
    double dz; //half size in z
};
class ATypeMonitorObject : public ATypeObject
{
public:
    ATypeMonitorObject() {Type = "Monitor"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json);
    virtual void readFromJson(QJsonObject& json);

    AMonitorConfig config;

    bool isParticleInUse(int partId) const;

    //runtime
    int index;  //index of monitor to fill and acess statistics
};


// ============== Virtual shape =================

class AGeoShape
{
public:
  AGeoShape() {}
  virtual ~AGeoShape() {}
  virtual bool readFromString(QString /*GenerationString*/) {return false;}

  //general: the same for all objects of the given shape
  virtual const QString getShapeType() {return "";}
  virtual const QString getShapeTemplate() {return "";}  //string used in auto generated help: Name(paramType param1, paramType param2, etc)
  virtual const QString getHelp() {return "";}

  //specific for this particular object
  virtual TGeoShape* createGeoShape(const QString /*shapeName*/ = "") {return 0;} //create ROOT TGeoShape
  virtual const QString getGenerationString() const {return "";} //return string which can be used to generate this object: e.g. Name(param1, param2, etc)
  virtual double getHeight() {return 0;}  //if 0, cannot be used for stack  ***!!!
  virtual void setHeight(double /*dz*/) {}
  virtual double maxSize() {return 0;} //for world size evaluation

  //json
  virtual void writeToJson(QJsonObject& /*json*/) {}
  virtual void readFromJson(QJsonObject& /*json*/) {}

  //from Tshape if geometry was loaded from GDML
  virtual bool readFromTShape(TGeoShape* /*Tshape*/) {return false;}

protected:
  bool extractParametersFromString(QString GenerationString, QStringList& parameters, int numParameters);

};

// -------------- Particular shapes ---------------

class AGeoParaboloid : public AGeoShape
{
public:
  AGeoParaboloid(double rlo, double rhi, double dz) :
    rlo(rlo), rhi(rhi), dz(dz) {}
  AGeoParaboloid() :
    rlo(0), rhi(40), dz(10)  {}
  virtual ~AGeoParaboloid() {}

  virtual const QString getShapeType() {return "TGeoParaboloid";}
  virtual const QString getShapeTemplate() {return "TGeoParaboloid( rlo, rhi, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double /*dz*/) {}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  //paraboloid specific
  double rlo, rhi, dz;
};

class AGeoConeSeg : public AGeoShape
{
public:
  AGeoConeSeg(double dz, double rminL, double rmaxL, double rminU, double rmaxU, double phi1, double phi2) :
    dz(dz), rminL(rminL), rmaxL(rmaxL), rminU(rminU), rmaxU(rmaxU), phi1(phi1), phi2(phi2) {}
  AGeoConeSeg() :
    dz(10), rminL(0), rmaxL(20), rminU(0), rmaxU(20), phi1(0), phi2(360) {}
  virtual ~AGeoConeSeg() {}

  virtual const QString getShapeType() {return "TGeoConeSeg";}
  virtual const QString getShapeTemplate() {return "TGeoConeSeg( dz, rminL, rmaxL, rminU, rmaxU, phi1, phi2 )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape *Tshape);

  double dz;
  double rminL, rmaxL, rminU, rmaxU;
  double phi1, phi2;
};

class AGeoCone : public AGeoShape
{
public:
  AGeoCone(double dz, double rminL, double rmaxL, double rminU, double rmaxU) :
    dz(dz), rminL(rminL), rmaxL(rmaxL), rminU(rminU), rmaxU(rmaxU) {}
  AGeoCone(double dz, double rmaxL, double rmaxU) :
    dz(dz), rminL(0), rmaxL(rmaxL), rminU(0), rmaxU(rmaxU) {}
  AGeoCone() :
    dz(10), rminL(0), rmaxL(20), rminU(0), rmaxU(20) {}
  virtual ~AGeoCone() {}

  virtual const QString getShapeType() {return "TGeoCone";}
  virtual const QString getShapeTemplate() {return "TGeoCone( dz, rminL, rmaxL, rminU, rmaxU )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dz;
  double rminL, rmaxL, rminU, rmaxU;
};

class AGeoPolygon : public AGeoShape
{
public:
  AGeoPolygon(int nedges, double dphi, double dz, double rminL, double rmaxL, double rminU, double rmaxU) :
    nedges(nedges), dphi(dphi), dz(dz), rminL(rminL), rmaxL(rmaxL), rminU(rminU), rmaxU(rmaxU) {}
  AGeoPolygon(int nedges, double dz, double rmaxL, double rmaxU) :
    nedges(nedges), dphi(360), dz(dz), rminL(0), rmaxL(rmaxL), rminU(0), rmaxU(rmaxU) {}
  AGeoPolygon() :
    nedges(6), dphi(360), dz(10), rminL(0), rmaxL(20), rminU(0), rmaxU(20) {}
  virtual ~AGeoPolygon() {}

  virtual const QString getShapeType() {return "TGeoPolygon";}
  virtual const QString getShapeTemplate() {return "TGeoPolygon( nedges, dphi, dz, rminL, rmaxL, rminU, rmaxU )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* /*Tshape*/) { return false; } //it is not a base root class, so not valid for import from GDML

  int nedges;
  double dphi, dz;
  double rminL, rmaxL, rminU, rmaxU;
};

struct APolyCGsection
{
  double z;
  double rmin, rmax;

  APolyCGsection() : z(0), rmin(0), rmax(100) {}
  APolyCGsection(double z, double rmin, double rmax) : z(z), rmin(rmin), rmax(rmax) {}

  bool fromString(QString string);
  const QString toString() const;
  void writeToJson(QJsonObject& json) const;
  void readFromJson(QJsonObject& json);
};

class AGeoPcon : public AGeoShape
{
public:
  AGeoPcon() : phi(0), dphi(360) {}
  virtual ~AGeoPcon() {}

  virtual const QString getShapeType() {return "TGeoPcon";}
  virtual const QString getShapeTemplate() {return "TGeoPcon( phi, dphi, { z0 : rmin0 : rmaz0 }, { z1 : rmin1 : rmax1 } )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double phi, dphi;
  QVector<APolyCGsection> Sections;

};

class AGeoPgon : public AGeoPcon
{
public:
  AGeoPgon() :
    AGeoPcon(), nedges(6) {}
  virtual ~AGeoPgon() {}

  virtual const QString getShapeType() {return "TGeoPgon";}
  virtual const QString getShapeTemplate() {return "TGeoPgon( phi, dphi, nedges, { z0 : rmin0 : rmaz0 }, { zN : rminN : rmaxN } )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  int nedges;
};

class AGeoTrd1 : public AGeoShape
{
public:
  AGeoTrd1(double dx1, double dx2, double dy, double dz) :
    dx1(dx1), dx2(dx2), dy(dy), dz(dz) {}
  AGeoTrd1() :
    dx1(15), dx2(5), dy(10), dz(10) {}
  virtual ~AGeoTrd1() {}

  virtual const QString getShapeType() {return "TGeoTrd1";}
  virtual const QString getShapeTemplate() {return "TGeoTrd1( dx1, dx2, dy, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dx1, dx2, dy, dz;
};

class AGeoTrd2 : public AGeoShape
{
public:
  AGeoTrd2(double dx1, double dx2, double dy1, double dy2, double dz) :
    dx1(dx1), dx2(dx2), dy1(dy1), dy2(dy2), dz(dz) {}
  AGeoTrd2() :
    dx1(15), dx2(5), dy1(10), dy2(20), dz(10) {}
  virtual ~AGeoTrd2() {}

  virtual const QString getShapeType() {return "TGeoTrd2";}
  virtual const QString getShapeTemplate() {return "TGeoTrd2( dx1, dx2, dy1, dy2, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dx1, dx2, dy1, dy2, dz;
};


class AGeoTube : public AGeoShape
{
public:
  AGeoTube(double rmin, double rmax, double dz) :
    rmin(rmin), rmax(rmax), dz(dz) {}
  AGeoTube(double r, double dz) :
    rmin(0), rmax(r), dz(dz) {}
  AGeoTube() :
    rmin(0), rmax(10), dz(5) {}
  virtual ~AGeoTube() {}

  virtual const QString getShapeType() {return "TGeoTube";}
  virtual const QString getShapeTemplate() {return "TGeoTube( rmin, rmax, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, dz;
};

class AGeoTubeSeg : public AGeoShape
{
public:
  AGeoTubeSeg(double rmin, double rmax, double dz, double phi1, double phi2) :
    rmin(rmin), rmax(rmax), dz(dz), phi1(phi1), phi2(phi2) {}
  AGeoTubeSeg() :
    rmin(0), rmax(10), dz(5), phi1(0), phi2(180) {}
  virtual ~AGeoTubeSeg() {}

  virtual const QString getShapeType() {return "TGeoTubeSeg";}
  virtual const QString getShapeTemplate() {return "TGeoTubeSeg( rmin, rmax, dz, phi1, phi2 )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, dz, phi1, phi2;
};

class AGeoCtub : public AGeoShape
{
public:
  AGeoCtub(double rmin, double rmax, double dz, double phi1, double phi2,
           double nxlow, double nylow, double nzlow,
           double nxhi, double nyhi, double nzhi) :
    rmin(rmin), rmax(rmax), dz(dz), phi1(phi1), phi2(phi2),
    nxlow(nxlow), nylow(nylow), nzlow(nzlow),
    nxhi(nxhi), nyhi(nyhi), nzhi(nzhi) {}
  AGeoCtub() :
    rmin(0), rmax(10), dz(5), phi1(0), phi2(180),
    nxlow(0), nylow(0.64), nzlow(-0.77),
    nxhi(0), nyhi(0.09), nzhi(0.87) {}
  virtual ~AGeoCtub() {}

  virtual const QString getShapeType() {return "TGeoCtub";}
  virtual const QString getShapeTemplate() {return "TGeoCtub( rmin, rmax, dz, phi1, phi2, nxlow, nylow, nzlow, nxhi, nyhi, nzhi )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, dz, phi1, phi2;
  double nxlow, nylow, nzlow;
  double nxhi, nyhi, nzhi;
};

class AGeoEltu : public AGeoShape
{
public:
  AGeoEltu(double a, double b, double dz) :
    a(a), b(b), dz(dz) {}
  AGeoEltu() :
    a(10), b(20), dz(5) {}
  virtual ~AGeoEltu() {}

  virtual const QString getShapeType() {return "TGeoEltu";}
  virtual const QString getShapeTemplate() {return "TGeoEltu( a, b, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() { return dz; }
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double a, b, dz;
};

class AGeoSphere : public AGeoShape
{
public:
  AGeoSphere(double rmin, double rmax, double theta1, double theta2, double phi1, double phi2) :
    rmin(rmin), rmax(rmax), theta1(theta1), theta2(theta2), phi1(phi1), phi2(phi2) {}
  AGeoSphere(double r) :
    rmin(0), rmax(r), theta1(0), theta2(180), phi1(0), phi2(360) {}
  AGeoSphere() :
    rmin(0), rmax(10), theta1(0), theta2(180), phi1(0), phi2(360) {}
  virtual ~AGeoSphere() {}

  virtual const QString getShapeType() {return "TGeoSphere";}
  virtual const QString getShapeTemplate() {return "TGeoSphere( rmin,  rmax, theta1, theta2, phi1, phi2 )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return rmax;}
  virtual void setHeight(double dz) {rmax = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize() { return rmax;}

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, theta1, theta2, phi1, phi2;
};

class AGeoBox : public AGeoShape
{
public:
  AGeoBox(double dx, double dy, double dz) :
    dx(dx), dy(dy), dz(dz) {}
  AGeoBox() :
    dx(10), dy(10), dz(10) {}
  virtual ~AGeoBox() {}

  virtual const QString getShapeType() {return "TGeoBBox";}
  virtual const QString getShapeTemplate() {return "TGeoBBox( dx, dy, dz )";}
  virtual bool readFromString(QString GenerationString);
  virtual const QString getHelp();

  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dx, dy, dz;
};

class AGeoPara : public AGeoShape
{
public:
  AGeoPara(double dx, double dy, double dz, double alpha, double theta, double phi) :
    dx(dx), dy(dy), dz(dz), alpha(alpha), theta(theta), phi(phi) {}
  AGeoPara() :
    dx(10), dy(10), dz(10), alpha(10), theta(25), phi(45) {}
  virtual ~AGeoPara() {}

  virtual const QString getShapeType() {return "TGeoPara";}
  virtual const QString getShapeTemplate() {return "TGeoPara( dX, dY, dZ, alpha, theta, phi )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dx, dy, dz;
  double alpha, theta, phi;
};

class AGeoArb8 : public AGeoShape
{
public:
  AGeoArb8(double dz, QList<QPair<double, double> > VertList);
  AGeoArb8();
  virtual ~AGeoArb8() {}

  virtual const QString getShapeType() {return "TGeoArb8";}
  virtual const QString getShapeTemplate() {return "TGeoArb8( dz,  xL1,yL1, xL2,yL2, xL3,yL3, xL4,yL4, xU1,yU1, xU2,yU2, xU3,yU3, xU4,yU4  )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dz;
  QList<QPair<double, double> > Vertices;

private:
  void init();

};

class AGeoComposite : public AGeoShape
{
public:
  AGeoComposite(const QStringList members, const QString GenerationString);
  AGeoComposite() {}
  virtual ~AGeoComposite() {}

  virtual const QString getShapeType() {return "TGeoCompositeShape";}
  virtual const QString getShapeTemplate() {return "TGeoCompositeShape( (A + B) * (C - D) )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  //virtual double getHeight() {return 0;}
  //virtual void setHeight(double /*dz*/) {}
  virtual const QString getGenerationString() const {return GenerationString;}
  virtual double maxSize() {return 0;}  //***!!!

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* /*Tshape*/) {return false;} //cannot be retrieved this way! need cooperation with AGeoObject itself

  QStringList members;
  QString GenerationString;
};

class AGeoScaledShape  : public AGeoShape
{
public:
  AGeoScaledShape(QString BaseShapeGenerationString, double scaleX, double scaleY, double scaleZ);
  AGeoScaledShape() {}
  virtual ~AGeoScaledShape() {}

  virtual const QString getShapeType() {return "TGeoScaledShape";}
  virtual const QString getShapeTemplate() {return "TGeoScaledShape( TGeoShape(parameters), scaleX, scaleY, scaleZ )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  //virtual double getHeight() {return 0;}
  //virtual void setHeight(double /*dz*/) {}
  virtual const QString getGenerationString() const;
  virtual double maxSize() {return 0;}  //***!!!

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  QString BaseShapeGenerationString;
  double scaleX, scaleY, scaleZ;

private:
  TGeoShape* generateBaseTGeoShape(QString BaseShapeGenerationString);
};

class AGeoTorus  : public AGeoShape
{
public:
  AGeoTorus(double R, double Rmin, double Rmax, double Phi1, double Dphi) :
    R(R), Rmin(Rmin), Rmax(Rmax), Phi1(Phi1), Dphi(Dphi) {}
  AGeoTorus() {}
  virtual ~AGeoTorus () {}

  virtual const QString getShapeType() {return "TGeoTorus";}
  virtual const QString getShapeTemplate() {return "TGeoTorus( R, Rmin, Rmax, Phi1, Dphi )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return Rmax;}
  virtual void setHeight(double dz) {this->Rmax = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  virtual void writeToJson(QJsonObject& json);
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double R;
  double Rmin, Rmax;
  double Phi1, Dphi;
};

#endif // AGEOOBJECT_H
