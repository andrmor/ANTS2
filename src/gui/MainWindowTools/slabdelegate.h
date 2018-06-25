#ifndef SLABDELEGATE_H
#define SLABDELEGATE_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>

class QCheckBox;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QFrame;
class QListWidgetItem;

// ======= MODELS =======
class ASlabXYModel;
class ASlabModel;

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

#endif // SLABDELEGATE_H
