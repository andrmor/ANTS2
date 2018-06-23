#ifndef ASLAB_H
#define ASLAB_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

// ======= MODELS =======
class ASlabXYModel
{
public:
  ASlabXYModel(int shape, int sides, double size1, double size2, double angle)
    : shape(shape), sides(sides), size1(size1), size2(size2), angle(angle) {}
  ASlabXYModel() : shape(0), sides(6), size1(100), size2(100), angle(0) {}

  int shape;
  int sides;
  double size1, size2;
  double angle;

  inline bool operator==(const ASlabXYModel& other) const
  {
    if (other.shape!=shape) return false;
    if (other.sides!=sides) return false;
    if (other.size1!=size1) return false;
    if (other.size2!=size2) return false;
    if (other.angle!=angle) return false;
    return true;
  }
  inline bool operator!=(const ASlabXYModel& other) const
  {
    return !(*this == other);
  }

  inline bool isSameShape(const ASlabXYModel& other) const
  {
    if (other.shape!=shape) return false;
    if (other.sides!=sides) return false;
    if (other.angle!=angle) return false;
    return true;
  }

  void writeToJson(QJsonObject &json);
  void readFromJson(QJsonObject &json);
};

class ASlabModel
{
public:
  ASlabModel(bool fActive, QString name, double height, int material, bool fCenter,
                           int shape, int sides, double size1, double size2, double angle)
    : fActive(fActive), name(name), height(height), material(material), fCenter(fCenter)
      {XYrecord.shape=shape; XYrecord.sides=sides; XYrecord.size1=size1; XYrecord.size2=size2; XYrecord.angle=angle;
       color = -1; style = 1; width = 1;}
  ASlabModel() : fActive(true), name(randomSlabName()), height(10), material(-1), fCenter(false)
     {color = -1; style = 1; width = 1;}

  bool fActive;
  QString name;
  double height;
  int material;
  bool fCenter;
  ASlabXYModel XYrecord;

  //visualization properties
  int color; // initializaed as -1, updated when first time shown by SlabListWidget
  int style;
  int width;

  inline bool operator==(const ASlabModel& other) const  //Z and line properties are not checked!
  {
    if (other.fActive!=fActive) return false;
    if (other.name!=name) return false;
    if (other.height!=height) return false;
    if (other.material!=material) return false;
    if (other.fCenter!=fCenter) return false;
    if ( !(other.XYrecord==XYrecord) ) return false;
    return true;
  }
  inline bool operator!=(const ASlabModel& other) const
  {
    return !(*this == other);
  }

  void writeToJson(QJsonObject &json);
  void readFromJson(QJsonObject &json);
  void importFromOldJson(QJsonObject &json); //compatibility!

  static QString randomSlabName()
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

  void updateWorldSize(double &XYm)
  {
    if (XYrecord.size1 > XYm)
      XYm = XYrecord.size1;
    if (XYrecord.shape == 0 && XYrecord.size2 > XYm)
        XYm = XYrecord.size2;
  }
};

#endif // ASLAB_H
