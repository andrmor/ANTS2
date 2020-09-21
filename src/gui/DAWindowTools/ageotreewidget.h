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
  void SetLineAttributes(AGeoObject * obj);

public slots:
  void UpdateGui(QString selected = "");
  void onGridReshapeRequested(QString objName);
  void objectMembersToScript(AGeoObject *Master, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void objectToScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void commonSlabToScript(QString &script, const QString &identStr);
  void rebuildDetectorAndRestoreCurrentDelegate();  // used by geoConst widget

private slots:
  void onItemSelectionChanged();   // !*! bold is also used now for stack reference volume
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
  AGeoObject * World      = nullptr;
  AGeoWidget * EditWidget = nullptr;

  //base images for icons
  QImage Lock;
  QImage GroupStart, GroupMid, GroupEnd;
  QImage StackStart, StackMid, StackEnd;

  QColor BackgroundColor;
  bool   fSpecialGeoViewMode;

  QTreeWidgetItem * previousHoverItem = nullptr;
  const QTreeWidgetItem * movingItem  = nullptr;  // used only to prevent highlight of item under the moving one if it is the same as target

  void populateTreeWidget(QTreeWidgetItem *parent, AGeoObject *Container, bool fDisabled = false);
  void updateExpandState(QTreeWidgetItem* item); //recursive!
  void updateIcon(QTreeWidgetItem *item, AGeoObject *obj);
  void formStack(QList<QTreeWidgetItem *> selected);
  void markAsStackRefVolume(AGeoObject * obj);
  void addLightguide(bool upper);

  void menuActionAddNewObject(AGeoObject * ContObj, AGeoShape * shape);
  void menuActionCopyObject(AGeoObject * ObjToCopy);
  void ShowObject(AGeoObject * obj);
  void ShowObjectRecursive(AGeoObject * obj);
  void ShowObjectOnly(AGeoObject * obj);
  void menuActionEnableDisable(AGeoObject * obj);
  void menuActionLock();
  void menuActionUnlock();
  void menuActionLockAllInside(AGeoObject * obj);
  void menuActionUnlockAllInside(AGeoObject * obj);
  void menuActionRemove();
  void menuActionRemoveHostedObjects(AGeoObject * obj);
  void menuActionRemoveRecursively(AGeoObject * obj);
  void menuActionAddNewComposite(AGeoObject * ContObj);
  void menuActionAddNewArray(AGeoObject * ContObj);
  void menuActionAddNewGrid(AGeoObject * ContObj);
  void menuActionAddNewMonitor(AGeoObject * ContObj);

  const QString makeScriptString_basicObject(AGeoObject *obj, bool bExpandMaterials, bool usePython) const;
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
  void ObjectSelectionChanged(QString); // should be fired with empty string if selection does not contain a single item
  void RequestRebuildDetector();
  void RequestHighlightObject(QString name);
  void RequestFocusObject(QString name);
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
  void onObjectSelectionChanged(QString SelectedObject); //starts GUI update
  void onStartEditing();
  void onRequestChangeShape(AGeoShape * NewShape);
  void onRequestChangeSlabShape(int NewShape);
  void onMonitorRequestsShowSensitiveDirection();

  void onRequestShowCurrentObject();
  void onRequestScriptLineToClipboard();
  void onRequestScriptRecursiveToClipboard();
  void onRequestSetVisAttributes();

  AGeoObject * getCurrentObject() {return CurrentObject;}

  void onConfirmPressed();                                        // CONFIRM BUTTON PRESSED: performing copy from delegate to object
  void onCancelPressed();

private:
  void exitEditingMode();
  AGeoBaseDelegate * createAndAddSlabDelegate();
  AGeoBaseDelegate * createAndAddGeoObjectDelegate();
  AGeoBaseDelegate * createAndAddGridElementDelegate();
  AGeoBaseDelegate * createAndAddMonitorDelegate();

signals:
  void showMonitor(const AGeoObject* mon);
  void requestBuildScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void requestEnableGeoConstWidget(bool);
};

#endif // AGEOTREEWIDGET_H
