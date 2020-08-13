#ifndef ATYPEGEOOBJECT_H
#define ATYPEGEOOBJECT_H

#include "amonitorconfig.h"

#include <QString>

class QJsonObject;
class ASlabModel;

class ATypeGeoObject
{
public:
    virtual ~ATypeGeoObject() {}

    bool isHandlingStatic() const   {return Handling == "Static";}      //World
    bool isHandlingStandard() const {return Handling == "Standard";}
    bool isHandlingSet() const      {return Handling == "Set";}         //Group, Stack, Composite container
    bool isHandlingArray() const    {return Handling == "Array";}       //Array

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

    //virtual QString updateType() {}

    virtual void writeToJson(QJsonObject& json); // virtual: CALL THIS, then save additional properties the concrete type has
    virtual void readFromJson(QJsonObject& json) = 0;  // virtual: read additional properties the concrete type has

    static ATypeGeoObject* TypeObjectFactory(const QString Type);  // TYPE FACTORY !!!

protected:
    QString Type;
    QString Handling;
};

  //static objects
class ATypeWorldObject : public ATypeGeoObject
{
public:
    ATypeWorldObject() {Type = "World"; Handling = "Static";}

    bool bFixedSize = false; // used by World delegate in GUI

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeSlabObject : public ATypeGeoObject
{
public:
    ATypeSlabObject(); //Slab Static
    ~ATypeSlabObject();

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); } //no need to save slab, it will be assigned from outside
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
class ATypeGroupContainerObject : public ATypeGeoObject
{
public:
    ATypeGroupContainerObject() {Type = "Group"; Handling = "Set";}

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeStackContainerObject : public ATypeGeoObject
{
public:
    ATypeStackContainerObject() {Type = "Stack"; Handling = "Set";}

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeCompositeContainerObject : public ATypeGeoObject
{
public:
    ATypeCompositeContainerObject() {Type = "CompositeContainer"; Handling = "Set";}

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};

  // AddObjects - objects which are added to geometry using the standard procedure
class ATypeSingleObject : public ATypeGeoObject
{
public:
    ATypeSingleObject() {Type = "Single"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeCompositeObject : public ATypeGeoObject
{
public:
    ATypeCompositeObject() {Type = "Composite"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};

class ATypeArrayObject : public ATypeGeoObject
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

    QString updateType();
    bool isGeoConstInUse(const QRegExp & nameRegExp) const;
    void replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName);

    virtual void writeToJson(QJsonObject& json);
    virtual void readFromJson(QJsonObject& json);

    int numX, numY, numZ;
    double stepX, stepY, stepZ;
    QString strNumX, strNumY, strNumZ, strStepX, strStepY, strStepZ;
};

class ATypeGridObject : public ATypeGeoObject
{
public:
    ATypeGridObject() {Type = "Grid"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json) { ATypeGeoObject::writeToJson(json); }
    virtual void readFromJson(QJsonObject& ) {}
};
class ATypeGridElementObject : public ATypeGeoObject
{
public:
    ATypeGridElementObject() {Type = "GridElement"; Handling = "Standard"; shape = 1; size1 = 10; size2 = 10; dz = 5;}

    virtual void writeToJson(QJsonObject& json);
    virtual void readFromJson(QJsonObject& json);

    int shape; //0 : rectanglar-2wires, 1 : rectangular-crossed,  2 : hexagonal
    double size1, size2; //half sizes for rectangular, size1 is size of hexagon
    double dz; //half size in z
};
class ATypeMonitorObject : public ATypeGeoObject
{
public:
    ATypeMonitorObject() {Type = "Monitor"; Handling = "Standard";}

    virtual void writeToJson(QJsonObject& json);
    virtual void readFromJson(QJsonObject& json);

    AMonitorConfig config;

    bool isParticleInUse(int partId) const;

    //runtime
    int index;  //index of monitor to fill and access statistics
};

#endif // ATYPEGEOOBJECT_H
