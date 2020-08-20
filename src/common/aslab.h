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

  bool    fActive;
  QString name;
  double  height;
  int     material;
  bool    fCenter;

  QString strHeight;

  ASlabXYModel XYrecord;

  //visualization properties
  int color; // initialized as -1, updated when first time shown by SlabListWidget
  int style;
  int width;

  bool operator==(const ASlabModel& other) const;
  bool operator!=(const ASlabModel& other) const;

  void writeToJson(QJsonObject & json) const;
  void readFromJson(const QJsonObject & json);

  void importFromOldJson(QJsonObject &json); //compatibility!
  void updateWorldSize(double &XYm) const;

  static QString randomSlabName();
};

#endif // ASLAB_H
