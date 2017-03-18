#ifndef APMARRAYDATA_H
#define APMARRAYDATA_H

#include "apmposangtyperecord.h"

#include <QString>
#include <QVector>

class QJsonObject;

class APmArrayData
{
public:
  bool   fActive;
  int    Regularity;   //0-regular, 1-auto_z, 2-custom
  int    PMtype;
  int    Packing;
  double CenterToCenter;
  bool   fUseRings;
  int    NumX;
  int    NumY;
  int    NumRings;
 QString PositioningScript;

  QVector<APmPosAngTypeRecord> PositionsAnglesTypes; //stores PM data, which is used to build pmts - both in GeoManager and PMs module

  APmArrayData() {fActive = false;Regularity=0;PMtype=0;Packing=0;CenterToCenter=80.0;fUseRings=false;NumX=2;NumY=2;NumRings=1;PositioningScript="";}
  void writeToJson(QJsonObject &json);
  bool readFromJson(QJsonObject &json);
};

#endif // APMARRAYDATA_H
