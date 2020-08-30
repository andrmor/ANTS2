#ifndef AGEOTREEWIDGET_H
#define AGEOTREEWIDGET_H

#include <QObject>
#include <QString>
#include <QTreeWidget>
#include <QImage>
#include <QColor>
#include <QList>

class AGeoObject;
class AGeoShape;
class AGeoWidget;
class ASandwich;
class QVBoxLayout;
class QPushButton;
class AGeoBaseDelegate;
class QPoint;
class QTreeWidgetItem;
class QStringList;

class AGeoTreeWidget : public QTreeWidget
{
  Q_OBJECT

public:
  AGeoTreeWidget(ASandwich* Sandwich);
  AGeoWidget* GetEditWidget() {return EditWidget;}

  ASandwich * Sandwich;

  void SelectObjects(QStringList ObjectNames);
  void SetLineAttributes(QString ObjectName);


public slots:
  void UpdateGui(QString selected = "");
  void onGridReshapeRequested(QString objName);
  void objectMembersToScript(AGeoObject *Master, QString &script, int ident, bool bExpandMaterial, bool bRecursive);
  void objectToScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive);
  void commonSlabToScript(QString &script);
  void rebuildDetetctorAndRestoreCurrentDelegate();

private slots:
  void onItemSelectionChanged();
  void customMenuRequested(const QPoint &pos);  // CONTEXT MENU
  void onItemClicked(); //only to return to normal geo view mode!

  void onItemExpanded(QTreeWidgetItem* item);
  void onItemCollapsed(QTreeWidgetItem* item);

  void onRemoveTriggered();
  void onRemoveRecursiveTriggered();

protected:
  void dropEvent(QDropEvent *event);
  void dragEnterEvent(QDragEnterEvent* event);
  void dragMoveEvent(QDragMoveEvent* event);

private:
  AGeoObject *World;
  AGeoWidget *EditWidget;

  //base images for icons
  QImage Lock;
  QImage GroupStart, GroupMid, GroupEnd;
  QImage StackStart, StackMid, StackEnd;

  QColor BackgroundColor;
  bool fSpecialGeoViewMode;

  QTreeWidgetItem * previousHoverItem = nullptr;
  const QTreeWidgetItem * movingItem = nullptr;  // used only to prevent highlight of item under the moving one if it is the same as target

  void populateTreeWidget(QTreeWidgetItem *parent, AGeoObject *Container, bool fDisabled = false);
  void updateExpandState(QTreeWidgetItem* item); //recursive!
  void updateIcon(QTreeWidgetItem *item, AGeoObject *obj);
  void formSet(QList<QTreeWidgetItem *> selected, int option);
  void addLightguide(bool upper);

  void menuActionAddNewObject(QString ContainerName, AGeoShape * shape);
  void menuActionCopyObject(QString ObjToCopyName);
  void ShowObject(QString ObjectName);
  void ShowObjectRecursive(QString ObjectName);
  void ShowObjectOnly(QString ObjectName);
  void menuActionEnableDisable(QString ObjectName);
  void menuActionLock();
  void menuActionUnlock();
  void menuActionLockAllInside(QString ObjectName);
  void menuActionUnlockAllInside(QString ObjectName);
  void menuActionRemove();
  void menuActionRemoveHostedObjects(QString ObjectName);
  void menuActionRemoveRecursively(QString ObjectName);
  void menuActionAddNewComposite(QString ContainerName);
  void menuActionAddNewArray(QString ContainerName);
  void menuActionAddNewGrid(QString ContainerName);
  void menuActionAddNewMonitor(QString ContainerName);

  const QString makeScriptString_basicObject(AGeoObject *obj, bool bExpandMaterials) const;
  const QString makeScriptString_slab(AGeoObject *obj, bool bExpandMaterials, int ident) const;
  const QString makeScriptString_setCenterSlab(AGeoObject *obj) const;
  QString makeScriptString_arrayObject(AGeoObject *obj);
  const QString makeScriptString_monitorBaseObject(const AGeoObject *obj) const;
  const QString makeScriptString_monitorConfig(const AGeoObject *obj) const;
  QString makeScriptString_stackObjectStart(AGeoObject *obj);
  QString makeScriptString_groupObjectStart(AGeoObject *obj);
  QString makeScriptString_stackObjectEnd(AGeoObject *obj);
  QString makeLinePropertiesString(AGeoObject *obj);
  const QString makeScriptString_DisabledObject(AGeoObject *obj);

signals:
  void ObjectSelectionChanged(const QString); // should be fired with empty string if selection does not contain a single item  
  void RequestRebuildDetector();
  void RequestHighlightObject(QString name);
  void RequestShowObjectRecursive(QString name);
  void RequestNormalDetectorDraw();
  void RequestListOfParticles(QStringList &definedParticles);
  void RequestShowMonitor(const AGeoObject* mon);
};

class AGeoWidget : public QWidget
{
  Q_OBJECT

public:
  AGeoWidget(AGeoObject* World, AGeoTreeWidget* tw);
  //destructor does not delete Widget - it is handled by the layout

private:
  AGeoObject* World;
  AGeoTreeWidget* tw;

  AGeoObject* CurrentObject = nullptr;

  AGeoBaseDelegate* GeoDelegate = nullptr;

  QVBoxLayout *lMain;
  QVBoxLayout *ObjectLayout;
  QFrame      *frBottom;
  QPushButton *pbConfirm, *pbCancel;

  bool fIgnoreSignals = true;
  bool fEditingMode = false;

  void ClearGui();
  void UpdateGui();

public slots:
  void onObjectSelectionChanged(const QString SelectedObject); //starts GUI update  //why const? :)
  void onStartEditing();
  void onRequestChangeShape(AGeoShape * NewShape);
  void onRequestChangeSlabShape(int NewShape);
  //void OnCustomContextMenuTriggered_forMainObject(QPoint pos);
  void onMonitorRequestsShowSensitiveDirection();

  void onRequestShowCurrentObject();
  void onRequestScriptLineToClipboard();
  void onRequestScriptRecursiveToClipboard();
  void onRequestSetVisAttributes();

  AGeoObject * getCurrentObject() {return CurrentObject;}

  void onConfirmPressed();                                        // CONFIRM BUTTON PRESSED -> COPY FROM DELEGATE TO OBJECT
  void onCancelPressed();

private:
  void exitEditingMode();
  AGeoBaseDelegate * createAndAddSlabDelegate();
  AGeoBaseDelegate * createAndAddGeoObjectDelegate();
  AGeoBaseDelegate * createAndAddGridElementDelegate();
  AGeoBaseDelegate * createAndAddMonitorDelegate();
  bool checkDelegateValidity();

signals:
  void showMonitor(const AGeoObject* mon);
  void requestBuildScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive);
};

#endif // AGEOTREEWIDGET_H
