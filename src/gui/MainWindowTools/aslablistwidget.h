#ifndef SLABSLISTWIDGET_H
#define SLABSLISTWIDGET_H

#include <QListWidget>

class ASlabModel;
class ASlabXYModel;
class ASlabDelegate;
class ASlabXYDelegate;
class ASandwich;

class ASlabListWidget : public QListWidget
{
  Q_OBJECT
public:
  ASlabListWidget(ASandwich* Sandwich, QWidget* parent = 0);
  ~ASlabListWidget();

  ASlabXYDelegate* GetDefaultXYdelegate() {return DefaultXYdelegate;}

public slots:
  void UpdateGui();
  void ItemRequestsModelUpdate(QWidget* source);

private:
  ASandwich * Sandwich = nullptr;
  ASlabXYModel * tmpXY = nullptr;                //used to copy-paste XY properties
  ASlabXYDelegate * DefaultXYdelegate = nullptr; //this is the widget that controls default XY properties
                                                 //it is created during construction of the listwidget. Get it using GetDefaultXYdelegate()
  bool fIgnoreModelUpdateRequests = false;

  ASlabDelegate* AddNewSlabDelegate();
  void ReshapeDelegate(int row);  
  void UpdateDefaultXYdelegateVisibility();

protected:
  void dropEvent(QDropEvent *event);

private slots:
  void customMenuRequested(const QPoint &pos);  // RIGHT BUTTON MENU
  void onDefaultXYpropertiesUpdateRequested();
  void onItemDoubleClicked(QListWidgetItem* item);

signals:
  void SlabDoubleClicked(QString SlabName);
  void RequestHighlightObject(QString);
};

#endif // SLABSLISTWIDGET_H
