#ifndef ASLAB_H
#define ASLAB_H

#include <QString>

class QJsonObject;

class ASlabXYModel
{
public:
  ASlabXYModel(int shape, int sides, double size1, double size2, double angle);
  ASlabXYModel(){}

  int    shape = 0;
  int    sides = 6;
  double size1 = 100.0;
  double size2 = 100.0;
  double angle = 0;

  QString strSides, strSize1, strSize2, strAngle;

  bool operator==(const ASlabXYModel & other) const;
  bool operator!=(const ASlabXYModel & other) const;

  bool isSameShape(const ASlabXYModel & other) const;

  void writeToJson(QJsonObject & json) const;
  void readFromJson(const QJsonObject & json);
};

class ASlabModel
{
public:
  ASlabModel(bool fActive, QString name, double height, int material, bool fCenter,
                           int shape, int sides, double size1, double size2, double angle);
  ASlabModel();

  bool    fActive = true;
  QString name;
  double  height = 10.0;
  int     material = -1;
  bool    fCenter = false;

  QString strHeight;

  ASlabXYModel XYrecord;

  //visualization properties
  int color = -1; // initialized as -1, updated when first time shown by SlabListWidget
  int style = 1;
  int width = 1;

  bool operator==(const ASlabModel& other) const;
  bool operator!=(const ASlabModel& other) const;

  void writeToJson(QJsonObject & json) const;
  void readFromJson(const QJsonObject & json);

  void importFromOldJson(QJsonObject &json); //compatibility!
  void updateWorldSize(double &XYm) const;

  static QString randomSlabName();

  QString makeRectangularSlabScriptString();
  QString makeRoundSlabScriptString();
  QString makePolygonSlabScriptString();
  QString makeSlabScriptString(QString matStr);

};

#endif // ASLAB_H
