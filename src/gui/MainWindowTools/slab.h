#ifndef DETECTORLAYER_H
#define DETECTORLAYER_H

#include <QString>
#include <QWidget>
#include <QJsonObject>
#include <QJsonArray>

class QCheckBox;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QFrame;
class QListWidgetItem;

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

// ======= DELEGATES =======
class ASlabXYDelegate : public QWidget
{
  Q_OBJECT

public:
  enum ShowStates {ShowNothing = 0, ShowSize, ShowAll};
  friend class ASlabDelegate;

  ASlabXYDelegate();

  //widget owns the following gui elements:
  QComboBox* comShape;
  QSpinBox* sbSides;
  QLineEdit* ledSize1;
  QLineEdit* ledSize2;
  QLineEdit* ledAngle;

  void CopyFrom(const ASlabXYDelegate& Another);
  const ASlabXYModel GetData();
  void SetShowState(ASlabXYDelegate::ShowStates State);

private:
  QStringList Shapes;  //names of possible shapes to be shown in combobox
  ASlabXYDelegate::ShowStates ShowState;

public slots:
  void UpdateGui(const ASlabXYModel& ModelRecord);

private slots:
  void onUserAction() { emit RequestModelUpdate(); } //triggered on actions when user finished editing any property
  void onContentChanged(); //triggered on actions when user modifies GUI
  void updateComponentVisibility();

signals:
  void RequestModelUpdate();
  void ContentChanged();
};

class ASlabDelegate : public QWidget
{
  Q_OBJECT
public:
  ASlabDelegate(QWidget* parent = 0);
  ~ASlabDelegate();

  void UpdateModel(ASlabModel* record);
  const ASlabModel GetData();

  void SetColorIndicator(int color);

  //widget owns the following gui elements:
  QFrame* frMain;
    // Copy constructor should update all constent of all following:
  QCheckBox* cbOnOff;
  QLineEdit* leName;
  QLineEdit* ledHeight;
  QComboBox* comMaterial;
  QFrame* frCenterZ;
  bool fCenter; //the idea to use visibility of the frame failed: if tab is closed, even visible frame reports "invisible"
  ASlabXYDelegate* XYdelegate;

  QFrame* frColor;

  //static methods
  static QListWidgetItem* CreateNewWidgetItem(); //create a QListWidgetItem item to host a new delegate

private:
  int color;

public slots:
  void UpdateGui(const ASlabModel &ModelRecord);

private slots:
  void onUserAction(); //triggered on actions when user modifies GUI
  void onContentChanged() { emit ContentChanged(); }

signals:
  void RequestModelUpdate(QWidget*);
  void ContentChanged();
};

#endif // DETECTORLAYER_H
