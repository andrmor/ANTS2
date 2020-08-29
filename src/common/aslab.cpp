#include "aslab.h"
#include "ajsontools.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

#include "TROOT.h"
#include "TColor.h"

ASlabXYModel::ASlabXYModel(int shape, int sides, double size1, double size2, double angle)
    : shape(shape), sides(sides), size1(size1), size2(size2), angle(angle) {}

bool ASlabXYModel::operator==(const ASlabXYModel &other) const
{
    if (other.shape != shape) return false;
    if (other.sides != sides) return false;
    if (other.size1 != size1) return false;
    if (other.size2 != size2) return false;
    if (other.angle != angle) return false;
    return true;
}

bool ASlabXYModel::operator!=(const ASlabXYModel &other) const
{
    return !(*this == other);
}

bool ASlabXYModel::isSameShape(const ASlabXYModel &other) const
{
    if (other.shape != shape) return false;
    if (other.sides != sides) return false;
    if (other.angle != angle) return false;
    return true;
}

void ASlabXYModel::writeToJson(QJsonObject &json) const
{
    json["shape"] = shape;
    json["sides"] = sides;
    json["size1"] = size1;
    json["size2"] = size2;
    json["angle"] = angle;

    if (!strSides.isEmpty()) json["strSides"] = strSides;
    if (!strSize1.isEmpty()) json["strSize1"] = strSize1;
    if (!strSize2.isEmpty()) json["strSize2"] = strSize2;
    if (!strAngle.isEmpty()) json["strAngle"] = strAngle;
}

void ASlabXYModel::readFromJson(const QJsonObject & json)
{
    *this = ASlabXYModel(); //to load default values

    parseJson(json, "shape", shape);
    parseJson(json, "sides", sides);
    parseJson(json, "size1", size1);
    parseJson(json, "size2", size2);
    parseJson(json, "angle", angle);

    parseJson(json, "strSides", strSides);
    parseJson(json, "strSize1", strSize1);
    parseJson(json, "strSize2", strSize2);
    parseJson(json, "strAngle", strAngle);
}

ASlabModel::ASlabModel(bool fActive, QString name, double height, int material, bool fCenter, int shape, int sides, double size1, double size2, double angle)
    : fActive(fActive), name(name), height(height), material(material), fCenter(fCenter)
{XYrecord.shape=shape; XYrecord.sides=sides; XYrecord.size1=size1; XYrecord.size2=size2; XYrecord.angle=angle;}

ASlabModel::ASlabModel() : name(randomSlabName()) {}

bool ASlabModel::operator==(const ASlabModel &other) const  //Z and line properties are not checked!
{
    if (other.fActive!=fActive) return false;
    if (other.name!=name) return false;
    if (other.height!=height) return false;
    if (other.material!=material) return false;
    if (other.fCenter!=fCenter) return false;
    if ( !(other.XYrecord==XYrecord) ) return false;
    return true;
}

bool ASlabModel::operator!=(const ASlabModel &other) const
{
    return !(*this == other);
}

void ASlabModel::writeToJson(QJsonObject &json) const
{
    json["fActive"] = fActive;
    json["name"] = name;
    json["height"] = height;
    json["material"] = material;
    json["fCenter"] = fCenter;
    json["color"] = color;
    json["style"] = style;
    json["width"] = width;

    if (!strHeight.isEmpty()) json["strHeight"] = strHeight;

    QJsonObject XYjson;
    XYrecord.writeToJson(XYjson);
    json["XY"] = XYjson;
}

#include "ageoconsts.h"
void ASlabModel::readFromJson(const QJsonObject & json)
{
    *this = ASlabModel(); //to load default values

    parseJson(json, "fActive",  fActive);
    parseJson(json, "name",     name);
    parseJson(json, "height",   height);
    parseJson(json, "material", material);
    parseJson(json, "fCenter",  fCenter);

    if (json.contains("XY"))
    {
        QJsonObject XYjson = json["XY"].toObject();
        XYrecord.readFromJson(XYjson);
    }

    parseJson(json, "strHeight",  strHeight);

    parseJson(json, "color",  color);
    parseJson(json, "style",  style);
    parseJson(json, "width",  width);

    const AGeoConsts & GC = AGeoConsts::getConstInstance();
    double sides;
    if (!XYrecord.strSides.isEmpty())
    {
        bool ok = GC.evaluateFormula(XYrecord.strSides, sides);
        if (ok && sides > 2) XYrecord.sides = sides;
    }
    if (!XYrecord.strSize1.isEmpty()) GC.evaluateFormula(XYrecord.strSize1, XYrecord.size1);
    if (!XYrecord.strSize2.isEmpty()) GC.evaluateFormula(XYrecord.strSize2, XYrecord.size2);
    if (!XYrecord.strAngle.isEmpty()) GC.evaluateFormula(XYrecord.strAngle, XYrecord.angle);
    if (!strHeight.isEmpty())         GC.evaluateFormula(strHeight,         height);
}

void ASlabModel::importFromOldJson(QJsonObject &json)
{
  *this = ASlabModel(); //to load default values
  if (json.contains("Active")) fActive = json["Active"].toBool();
  if (json.contains("name")) name = "Undefined";
  if (json.contains("Height")) height = json["Height"].toDouble();
  if (json.contains("Material")) material = json["Material"].toInt();

  if (json.contains("Shape")) XYrecord.shape = json["Shape"].toInt();
  if (json.contains("Sides")) XYrecord.sides = json["Sides"].toInt();
  if (json.contains("Size1")) XYrecord.size1 = json["Size1"].toDouble();
  if (json.contains("Size2")) XYrecord.size2 = json["Size2"].toDouble();
  if (json.contains("Angle")) XYrecord.angle = json["Angle"].toDouble();

  //name and center status has to be given on detector level - they depend on index
}

QString ASlabModel::randomSlabName()
{
    const QString possibleLett("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    const QString possibleNum("0123456789");
    const int lettLength = 1;
    const int numLength = 1;

    QString randomString;
    for(int i=0; i<lettLength; i++)
    {
        int index = qrand() % possibleLett.length();
        QChar nextChar = possibleLett.at(index);
        randomString.append(nextChar);
    }
    for(int i=0; i<numLength; i++)
    {
        int index = qrand() % possibleNum.length();
        QChar nextChar = possibleNum.at(index);
        randomString.append(nextChar);
    }
    randomString = "New_"+randomString;
    return randomString;
}

QString ASlabModel::makeRectangularSlabScriptString()
{
    ASlabXYModel &xy = XYrecord;
    return QString(QString::number(height)) + ", "+
                   QString::number(xy.size1) + ", "+
                   QString::number(xy.size2) + ", "+
                   QString::number(xy.angle) + ")";
}


QString ASlabModel::makeRoundSlabScriptString()
{
    ASlabXYModel &xy = XYrecord;
    return QString(QString::number(height)) + "', "+
                   QString::number(xy.size1) + ")";
}

QString ASlabModel::makePolygonSlabScriptString()
{
    ASlabXYModel &xy = XYrecord;
    return QString(QString::number(height)) + ", "+
                   QString::number(xy.size1) + ", "+
                   QString::number(xy.angle) + ", "+
                   QString::number(xy.sides) + ")";
}

QString ASlabModel::makeSlabScriptString()
{
    ASlabXYModel &xy = XYrecord;
    switch(xy.shape)
    {
        case 0:
            return QString("geo.SlabRectangular( ") +
                    "'" + name + "', " +
                    materialPlaceholderStr + ", " +
                    makeRectangularSlabScriptString();
        case 1:
            return QString("geo.SlabRound( ") +
                    "'" + name + "', " +
                    materialPlaceholderStr + ", " +
                    makeRoundSlabScriptString();
        case 2:
            return QString("geo.SlabPolygon( ") +
                    "'" + name + "', " +
                    materialPlaceholderStr + ", " +
                    makePolygonSlabScriptString();
    }
    return "slab shape not found";
}

void ASlabModel::updateWorldSize(double & XYm) const
{
    if (XYrecord.size1 > XYm)
        XYm = XYrecord.size1;
    if (XYrecord.shape == 0 && XYrecord.size2 > XYm)
        XYm = XYrecord.size2;
}
