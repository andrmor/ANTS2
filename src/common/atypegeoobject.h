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

    // TODO String -> enum
    bool isHandlingStatic() const   {return Handling == "Static";}      //World
    bool isHandlingStandard() const {return Handling == "Standard";}
    bool isHandlingSet() const      {return Handling == "Set";}         //Group, Stack, Composite container
    bool isHandlingArray() const    {return Handling == "Array";}       //Array

    bool isWorld() const            {return Type == "World";}
    bool isPrototypes() const       {return Type == "PrototypeCollection";}
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
    bool isInstance() const         {return Type == "Instance";}
    bool isPrototype() const        {return Type == "Prototype";}
    bool isGrid() const             {return Type == "Grid";}
    bool isGridElement() const      {return Type == "GridElement";}
    bool isMonitor() const          {return Type == "Monitor";}

    virtual bool isGeoConstInUse(const QRegExp & /*nameRegExp*/) const {return false;}
    virtual void replaceGeoConstName(const QRegExp & /*nameRegExp*/, const QString & /*newName*/) {}

    virtual void writeToJson(QJsonObject & json) const;         // CALL THIS, then save additional properties of the concrete type
    virtual void readFromJson(const QJsonObject & /*json*/) {}  // if present, read properties of the concrete type

    static ATypeGeoObject * TypeObjectFactory(const QString & Type);  // TYPE FACTORY !!!

protected:
    QString Type;
    QString Handling;
};


// -- static objects --

class ATypeWorldObject : public ATypeGeoObject
{
public:
    ATypeWorldObject() {Type = "World"; Handling = "Static";}

    bool bFixedSize = false;

    void writeToJson(QJsonObject & json) const override;
    void readFromJson(const QJsonObject & json) override;
};

class ATypePrototypeCollectionObject : public ATypeGeoObject
{
public:
    ATypePrototypeCollectionObject() {Type = "PrototypeCollection"; Handling = "Logical";}
};

class ATypeSlabObject : public ATypeGeoObject
{
public:
    ATypeSlabObject();
    ~ATypeSlabObject();

    bool isGeoConstInUse(const QRegExp & nameRegExp) const;
    void replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName);

    ASlabModel * SlabModel = nullptr;
};

class ATypeLightguideObject : public ATypeSlabObject  //inherits from Slab!
{
public:
    ATypeLightguideObject(); //Lightguide Static

    enum UpperLowerEnum {Upper = 0, Lower};

    void writeToJson(QJsonObject & json) const override; //no need to save slab, it will be assigned from outside
    void readFromJson(const QJsonObject & json) override;

    UpperLowerEnum UpperLower;
};


// -- Set objects --

class ATypeGroupContainerObject : public ATypeGeoObject
{
public:
    ATypeGroupContainerObject() {Type = "Group"; Handling = "Set";}
};

class ATypeStackContainerObject : public ATypeGeoObject
{
public:
    ATypeStackContainerObject() {Type = "Stack"; Handling = "Set";}

    QString ReferenceVolume;

    void writeToJson(QJsonObject & json) const override;
    void readFromJson(const QJsonObject & json) override;
};

class ATypeCompositeContainerObject : public ATypeGeoObject
{
public:
    ATypeCompositeContainerObject() {Type = "CompositeContainer"; Handling = "Set";}
};


// -- Objects --

class ATypeSingleObject : public ATypeGeoObject
{
public:
    ATypeSingleObject() {Type = "Single"; Handling = "Standard";}
};

class ATypeCompositeObject : public ATypeGeoObject
{
public:
    ATypeCompositeObject() {Type = "Composite"; Handling = "Standard";}
};

class ATypeArrayObject : public ATypeGeoObject
{
public:
    ATypeArrayObject() {Type = "Array"; Handling = "Array";}
    ATypeArrayObject(int numX, int numY, int numZ, double stepX, double stepY, double stepZ)
        : numX(numX), numY(numY), numZ(numZ), stepX(stepX), stepY(stepY), stepZ(stepZ) {Type = "Array"; Handling = "Array";}

    void Reconfigure(int NumX, int NumY, int NumZ, double StepX, double StepY, double StepZ);

    bool isGeoConstInUse(const QRegExp & nameRegExp) const override;
    void replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName) override;

    void writeToJson(QJsonObject & json) const override;
    void readFromJson(const QJsonObject & json) override;

    int numX = 2;
    int numY = 2;
    int numZ = 1;
    double stepX = 25.0;
    double stepY = 25.0;
    double stepZ = 25.0;
    QString strNumX, strNumY, strNumZ, strStepX, strStepY, strStepZ;
    int startIndex = 0; QString strStartIndex;

    static QString evaluateStringValues(ATypeArrayObject & ArrayType);
};

class ATypeGridObject : public ATypeGeoObject
{
public:
    ATypeGridObject() {Type = "Grid"; Handling = "Standard";}
};

class ATypeGridElementObject : public ATypeGeoObject
{
public:
    ATypeGridElementObject() {Type = "GridElement"; Handling = "Standard";}

    void writeToJson(QJsonObject & json) const override;
    void readFromJson(const QJsonObject & json) override;

    int    shape = 1;       //0 : rectanglar-2wires, 1 : rectangular-crossed,  2 : hexagonal
    double size1 = 10.0;    //half sizes for rectangular, size1 is size of hexagon
    double size2 = 10.0;
    double dz    = 5.0;     //half size in z
};

class ATypeMonitorObject : public ATypeGeoObject
{
public:
    ATypeMonitorObject() {Type = "Monitor"; Handling = "Standard";}

    void writeToJson(QJsonObject & json) const override;
    void readFromJson(const QJsonObject & json) override;

    bool isGeoConstInUse(const QRegExp & nameRegExp) const override;
    void replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName) override;

    AMonitorConfig config;

    bool isParticleInUse(int partId) const;

    //runtime, not saved
    int index;  //index of this monitor to access statistics
};

class ATypePrototypeObject : public ATypeGeoObject
{
public:
    ATypePrototypeObject() {Type = "Prototype"; Handling = "Set";}
};

class ATypeInstanceObject : public ATypeGeoObject
{
public:
    ATypeInstanceObject(QString PrototypeName = "") : PrototypeName(PrototypeName) {Type = "Instance"; Handling = "Standard";}

    void writeToJson(QJsonObject & json) const override;
    void readFromJson(const QJsonObject & json) override;

    QString PrototypeName;
};

#endif // ATYPEGEOOBJECT_H
