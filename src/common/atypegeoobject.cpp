#include "atypegeoobject.h"
#include "aslab.h"
#include "ajsontools.h"

#include <QDebug>

void ATypeGeoObject::writeToJson(QJsonObject &json)
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
    if (SlabModel) delete SlabModel;
    //qDebug() << "Done!!";
}

ATypeLightguideObject::ATypeLightguideObject()
{
    Type = "Lightguide";
    Handling = "Static";
}

void ATypeLightguideObject::writeToJson(QJsonObject &json)
{
    ATypeGeoObject::writeToJson(json);
    json["UpperLower"] = (UpperLower == Lower) ? "Lower" : "Upper";
}

void ATypeLightguideObject::readFromJson(QJsonObject &json)
{
    QString UppLow = json["UpperLower"].toString();
    UpperLower = (UppLow == "Lower") ? Lower : Upper;
}

void ATypeArrayObject::updateType()
{

}

void ATypeArrayObject::writeToJson(QJsonObject &json)
{
    ATypeGeoObject::writeToJson(json);
    json["numX"] = numX;
    json["numY"] = numY;
    json["numZ"] = numZ;
    json["stepX"] = stepX;
    json["stepY"] = stepY;
    json["stepZ"] = stepZ;
}

void ATypeArrayObject::readFromJson(QJsonObject &json)
{
    parseJson(json, "numX", numX);
    parseJson(json, "numY", numY);
    parseJson(json, "numZ", numZ);
    parseJson(json, "stepX", stepX);
    parseJson(json, "stepY", stepY);
    parseJson(json, "stepZ", stepZ);
}

void ATypeGridElementObject::writeToJson(QJsonObject &json)
{
    ATypeGeoObject::writeToJson(json);
    json["size1"] = size1;
    json["size2"] = size2;
    json["shape"] = shape;
    json["dz"] = dz;
}

void ATypeGridElementObject::readFromJson(QJsonObject &json)
{
    parseJson(json, "size1", size1);
    parseJson(json, "size2", size2);
    parseJson(json, "shape", shape);
    parseJson(json, "dz", dz);
}

void ATypeMonitorObject::writeToJson(QJsonObject &json)
{
    ATypeGeoObject::writeToJson(json);
    config.writeToJson(json);
}

void ATypeMonitorObject::readFromJson(QJsonObject &json)
{
    config.readFromJson(json);
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

ATypeGeoObject *ATypeGeoObject::TypeObjectFactory(const QString Type)
{
    if (Type == "World")
        return new ATypeWorldObject(); //is not used to create World, only to check file with WorldTree starts with World and reads positioning script
    if (Type == "Slab")
        return new ATypeSlabObject();
    if (Type == "Lightguide")
        return new ATypeLightguideObject();
    else if (Type == "Group")
        return new ATypeGroupContainerObject();
    else if (Type == "Stack")
        return new ATypeStackContainerObject();
    else if (Type == "CompositeContainer")
        return new ATypeCompositeContainerObject();
    else if (Type == "Single")
        return new ATypeSingleObject();
    else if (Type == "Composite")
        return new ATypeCompositeObject();
    else if (Type == "Array" || Type == "XYArray")
        return new ATypeArrayObject();
    else if (Type == "Grid")
        return new ATypeGridObject();
    else if (Type == "GridElement")
        return new ATypeGridElementObject();
    else if (Type == "Monitor")
        return new ATypeMonitorObject();
    else
    {
        qCritical() << "Unknown opject type in TypeObjectFactory:"<<Type;
        return 0;
    }
}
