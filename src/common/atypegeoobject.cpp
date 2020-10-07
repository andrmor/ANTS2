#include "atypegeoobject.h"
#include "aslab.h"
#include "ajsontools.h"
#include "ageoconsts.h"

#include <QDebug>

void ATypeGeoObject::writeToJson(QJsonObject & json) const
{
    json["Type"] = Type;
}

ATypeSlabObject::ATypeSlabObject()
{
    Type = "Slab";
    Handling = "Static";
    SlabModel = new ASlabModel();
}

ATypeSlabObject::~ATypeSlabObject()
{
    //qDebug() << "Destructor called for slab object inside:"<< ( SlabModel ? SlabModel->name : "-Model is NULL-");
    delete SlabModel;
    //qDebug() << "Done!!";
}

bool ATypeSlabObject::isGeoConstInUse(const QRegExp & nameRegExp) const
{
    if (!SlabModel) return false;

    if (SlabModel->strHeight.contains(nameRegExp)) return true;
    ASlabXYModel & XY = SlabModel->XYrecord;
    if (XY.strSides.contains(nameRegExp)) return true;
    if (XY.strSize1.contains(nameRegExp)) return true;
    if (XY.strSize2.contains(nameRegExp)) return true;
    if (XY.strAngle.contains(nameRegExp)) return true;

    return false;
}

void ATypeSlabObject::replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName)
{
    if (!SlabModel) return;

    SlabModel->strHeight.replace(nameRegExp, newName);
    ASlabXYModel & XY = SlabModel->XYrecord;
    XY.strSides.replace(nameRegExp, newName);
    XY.strSize1.replace(nameRegExp, newName);
    XY.strSize2.replace(nameRegExp, newName);
    XY.strAngle.replace(nameRegExp, newName);
}

ATypeLightguideObject::ATypeLightguideObject()
{
    Type = "Lightguide";
    Handling = "Static";
}

void ATypeLightguideObject::writeToJson(QJsonObject &json) const
{
    ATypeGeoObject::writeToJson(json);

    json["UpperLower"] = ( UpperLower == Lower ? "Lower" : "Upper" );
}

void ATypeLightguideObject::readFromJson(const QJsonObject &json)
{
    QString UppLow = json["UpperLower"].toString();
    UpperLower = (UppLow == "Lower" ? Lower : Upper);
}

void ATypeArrayObject::Reconfigure(int NumX, int NumY, int NumZ, double StepX, double StepY, double StepZ)
{
    numX = NumX; numY = NumY; numZ = NumZ;
    stepX = StepX; stepY = StepY; stepZ = StepZ;
}

QString ATypeArrayObject::evaluateStringValues(ATypeArrayObject & A)
{
    const AGeoConsts & GC = AGeoConsts::getConstInstance();

    QString errorStr;
    bool ok;

    ok = GC.updateParameter(errorStr, A.strNumX, A.numX, true, true) ; if (!ok) return errorStr;
    ok = GC.updateParameter(errorStr, A.strNumY, A.numY, true, true) ; if (!ok) return errorStr;
    ok = GC.updateParameter(errorStr, A.strNumZ, A.numZ, true, true) ; if (!ok) return errorStr;

    ok = GC.updateParameter(errorStr, A.strStepX, A.stepX, true, false, false) ; if (!ok) return errorStr;
    ok = GC.updateParameter(errorStr, A.strStepY, A.stepY, true, false, false) ; if (!ok) return errorStr;
    ok = GC.updateParameter(errorStr, A.strStepZ, A.stepZ, true, false, false) ; if (!ok) return errorStr;

    ok = GC.updateParameter(errorStr, A.strStartIndex, A.startIndex, false, true) ; if (!ok) return errorStr;

    return "";
}

bool ATypeArrayObject::isGeoConstInUse(const QRegExp &nameRegExp) const
{
    if (strNumX .contains(nameRegExp)) return true;
    if (strNumY .contains(nameRegExp)) return true;
    if (strNumZ .contains(nameRegExp)) return true;
    if (strStepX.contains(nameRegExp)) return true;
    if (strStepY.contains(nameRegExp)) return true;
    if (strStepZ.contains(nameRegExp)) return true;
    if (strStartIndex.contains(nameRegExp)) return true;
    return false;
}

void ATypeArrayObject::replaceGeoConstName(const QRegExp &nameRegExp, const QString &newName)
{
    strNumX .replace(nameRegExp, newName);
    strNumY .replace(nameRegExp, newName);
    strNumZ .replace(nameRegExp, newName);
    strStepX.replace(nameRegExp, newName);
    strStepY.replace(nameRegExp, newName);
    strStepZ.replace(nameRegExp, newName);
    strStartIndex.replace(nameRegExp, newName);
}

void ATypeArrayObject::writeToJson(QJsonObject &json) const
{
    ATypeGeoObject::writeToJson(json);

    json["numX"]  = numX;
    json["numY"]  = numY;
    json["numZ"]  = numZ;
    json["stepX"] = stepX;
    json["stepY"] = stepY;
    json["stepZ"] = stepZ;
    json["startIndex"] = startIndex;

    if (!strNumX .isEmpty()) json["strNumX"]  = strNumX;
    if (!strNumY .isEmpty()) json["strNumY"]  = strNumY;
    if (!strNumZ .isEmpty()) json["strNumZ"]  = strNumZ;
    if (!strStepX.isEmpty()) json["strStepX"] = strStepX;
    if (!strStepY.isEmpty()) json["strStepY"] = strStepY;
    if (!strStepZ.isEmpty()) json["strStepZ"] = strStepZ;
    if (!strStartIndex.isEmpty()) json["strStartIndex"] = strStartIndex;
}

void ATypeArrayObject::readFromJson(const QJsonObject &json)
{
    parseJson(json, "numX",  numX);
    parseJson(json, "numY",  numY);
    parseJson(json, "numZ",  numZ);
    parseJson(json, "stepX", stepX);
    parseJson(json, "stepY", stepY);
    parseJson(json, "stepZ", stepZ);
    parseJson(json, "startIndex", startIndex);

    if (!parseJson(json, "strNumX",  strNumX))  strNumX .clear();
    if (!parseJson(json, "strNumY",  strNumY))  strNumY .clear();
    if (!parseJson(json, "strNumZ",  strNumZ))  strNumZ .clear();
    if (!parseJson(json, "strStepX", strStepX)) strStepX.clear();
    if (!parseJson(json, "strStepY", strStepY)) strStepY.clear();
    if (!parseJson(json, "strStepZ", strStepZ)) strStepZ.clear();
    if (!parseJson(json, "strStartIndex", strStartIndex)) strStartIndex.clear();

    ATypeArrayObject::evaluateStringValues(*this);
}

void ATypeGridElementObject::writeToJson(QJsonObject &json) const
{
    ATypeGeoObject::writeToJson(json);

    json["size1"] = size1;
    json["size2"] = size2;
    json["shape"] = shape;
    json["dz"]    = dz;
}

void ATypeGridElementObject::readFromJson(const QJsonObject &json)
{
    parseJson(json, "size1", size1);
    parseJson(json, "size2", size2);
    parseJson(json, "shape", shape);
    parseJson(json, "dz",    dz);
}

void ATypeMonitorObject::writeToJson(QJsonObject &json) const
{
    ATypeGeoObject::writeToJson(json);

    config.writeToJson(json);
}

void ATypeMonitorObject::readFromJson(const QJsonObject &json)
{
    config.readFromJson(json);
}

bool ATypeMonitorObject::isGeoConstInUse(const QRegExp & nameRegExp) const
{
    if (config.str2size1.contains(nameRegExp)) return true;
    if (config.str2size2.contains(nameRegExp)) return true;
    return false;
}

void ATypeMonitorObject::replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName)
{
    config.str2size1.replace(nameRegExp, newName);
    config.str2size2.replace(nameRegExp, newName);
}

bool ATypeMonitorObject::isParticleInUse(int partId) const
{
    if (config.PhotonOrParticle == 0) return false;
    return (config.ParticleIndex == partId);
}

bool ATypeGeoObject::isUpperLightguide() const
{
    if ( Type != "Lightguide") return false;

    const ATypeLightguideObject* obj = static_cast<const ATypeLightguideObject*>(this);
    return obj->UpperLower == ATypeLightguideObject::Upper;
}

bool ATypeGeoObject::isLowerLightguide() const
{
    if ( Type != "Lightguide") return false;

    const ATypeLightguideObject* obj = static_cast<const ATypeLightguideObject*>(this);
    return obj->UpperLower == ATypeLightguideObject::Lower;
}

ATypeGeoObject *ATypeGeoObject::TypeObjectFactory(const QString & Type)
{
    if (Type == "Single")             return new ATypeSingleObject();
    if (Type == "Slab")               return new ATypeSlabObject();
    if (Type == "Array" ||
        Type == "XYArray")            return new ATypeArrayObject();
    if (Type == "Monitor")            return new ATypeMonitorObject();
    if (Type == "Stack")              return new ATypeStackContainerObject();
    if (Type == "Instance")           return new ATypeInstanceObject();
    if (Type == "Prototype")          return new ATypePrototypeObject();
    if (Type == "Composite")          return new ATypeCompositeObject();
    if (Type == "CompositeContainer") return new ATypeCompositeContainerObject();
    if (Type == "GridElement")        return new ATypeGridElementObject();
    if (Type == "Grid")               return new ATypeGridObject();
    if (Type == "Lightguide")         return new ATypeLightguideObject();
    if (Type == "Group")              return new ATypeGroupContainerObject();
    if (Type == "PrototypeCollection")return new ATypePrototypeCollectionObject();
    if (Type == "World")              return new ATypeWorldObject(); //is not used to create World, only to check file with WorldTree starts with World and reads positioning script

    qCritical() << "Unknown opject type in TypeObjectFactory:"<<Type;
    return nullptr;
}

void ATypeWorldObject::writeToJson(QJsonObject & json) const
{
    ATypeGeoObject::writeToJson(json);
    json["FixedSize"] = bFixedSize;
}

void ATypeWorldObject::readFromJson(const QJsonObject &json)
{
    bFixedSize = false;
    parseJson(json, "FixedSize", bFixedSize);
}

void ATypeStackContainerObject::writeToJson(QJsonObject & json) const
{
    ATypeGeoObject::writeToJson(json);
    json["ReferenceVolume"] = ReferenceVolume;
}

void ATypeStackContainerObject::readFromJson(const QJsonObject & json)
{
    ReferenceVolume.clear();
    parseJson(json, "ReferenceVolume", ReferenceVolume);
}

void ATypeInstanceObject::writeToJson(QJsonObject & json) const
{
    ATypeGeoObject::writeToJson(json);
    json["PrototypeName"] = PrototypeName;
}

void ATypeInstanceObject::readFromJson(const QJsonObject & json)
{
    PrototypeName.clear();
    parseJson(json, "PrototypeName", PrototypeName);
}
