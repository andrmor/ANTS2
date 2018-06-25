#include "aslab.h"

#include <QDebug>

//ROOT
#include "TROOT.h"
#include "TColor.h"

void ASlabXYModel::writeToJson(QJsonObject &json)
{
  json["shape"] = shape;
  json["sides"] = sides;
  json["size1"] = size1;
  json["size2"] = size2;
  json["angle"] = angle;
}

void ASlabXYModel::readFromJson(QJsonObject &json)
{
  *this = ASlabXYModel(); //to load default values
  if (json.contains("shape")) shape = json["shape"].toInt();
  if (json.contains("sides")) sides = json["sides"].toInt();
  if (json.contains("size1")) size1 = json["size1"].toDouble();
  if (json.contains("size2")) size2 = json["size2"].toDouble();
  if (json.contains("angle")) angle = json["angle"].toDouble();
}

void ASlabModel::writeToJson(QJsonObject &json)
{
  json["fActive"] = fActive;
  json["name"] = name;
  json["height"] = height;
  json["material"] = material;
  json["fCenter"] = fCenter;
  json["color"] = color;
  json["style"] = style;
  json["width"] = width;

  QJsonObject XYjson;
  XYrecord.writeToJson(XYjson);
  json["XY"] = XYjson;
}

void ASlabModel::readFromJson(QJsonObject &json)
{
  *this = ASlabModel(); //to load default values
  if (json.contains("fActive")) fActive = json["fActive"].toBool();
  if (json.contains("name")) name = json["name"].toString();
  if (json.contains("height")) height = json["height"].toDouble();
  if (json.contains("material")) material = json["material"].toInt();
  if (json.contains("fCenter")) fCenter = json["fCenter"].toBool();
  if (json.contains("XY"))
    {
      QJsonObject XYjson = json["XY"].toObject();
      XYrecord.readFromJson(XYjson);
    }
  if (json.contains("color")) color = json["color"].toInt();
  if (json.contains("style")) style = json["style"].toInt();
  if (json.contains("width")) width = json["width"].toInt();
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
