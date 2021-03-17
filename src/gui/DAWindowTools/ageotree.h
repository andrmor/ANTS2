#ifndef AGEOTREE_H
#define AGEOTREE_H

#include <QObject>
#include <QString>
#include <QTreeWidget>
#include <QImage>
#include <QColor>
#include <QList>

class AGeoBaseTreeWidget;
class AGeoObject;
class AGeoShape;
class AGeoDelegateWidget;
class ASandwich;
class QPoint;
class QTreeWidgetItem;
class QStringList;

class AGeoTree : public QObject
{
  Q_OBJECT

public:
  AGeoTree(ASandwich * Sandwich);

  AGeoDelegateWidget * GetEditWidget() {return EditWidget;}
  void         SetLineAttributes(AGeoObject * obj);
  void         SelectObjects(QStringList ObjectNames);
  void         AddLightguide(bool bUpper);

  AGeoBaseTreeWidget * twGeoTree    = nullptr;
  AGeoBaseTreeWidget * twPrototypes = nullptr;
  QString LastShownObjectName;

public slots:
  void UpdateGui(QString ObjectName = "");
  void onGridReshapeRequested(QString objName);
  void objectMembersToScript(AGeoObject *Master, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void objectToScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void commonSlabToScript(QString &script, const QString &identStr);
  void rebuildDetectorAndRestoreCurrentDelegate();  // used by geoConst widget
  void onRequestShowPrototype(QString ProtoName);
  void onRequestIsValidPrototypeName(const QString & ProtoName, bool & bResult) const;

private slots:
  void onItemSelectionChanged();
  void onProtoItemSelectionChanged();
  void customMenuRequested(const QPoint &pos);      // ==== World tree CONTEXT MENU ====
  void customProtoMenuRequested(const QPoint &pos); // ---- Proto tree CONTEXT MENU ----
  void onItemClicked();
  void onProtoItemClicked();

  void onItemExpanded(QTreeWidgetItem * item);
  void onItemCollapsed(QTreeWidgetItem * item);
  void onPrototypeItemExpanded(QTreeWidgetItem * item);
  void onPrototypeItemCollapsed(QTreeWidgetItem * item);

  void onRemoveTriggered();
  void onRemoveRecursiveTriggered();

private:
  ASandwich  * Sandwich   = nullptr;
  AGeoObject * World      = nullptr;
  AGeoObject * Prototypes = nullptr;

  AGeoDelegateWidget * EditWidget = nullptr;

  QTreeWidgetItem * topItemPrototypes = nullptr;
  bool bWorldTreeSelected = true;

  //base images for icons
  QImage Lock;
  //QImage GroupStart, GroupMid, GroupEnd;
  QImage StackStart, StackMid, StackEnd;

  //QColor BackgroundColor = QColor(240,240,240);
  bool   fSpecialGeoViewMode = false;

  void loadImages();
  void populateTreeWidget(QTreeWidgetItem *parent, AGeoObject *Container, bool fDisabled = false);
  void updateExpandState(QTreeWidgetItem * item, bool bPrototypes); //recursive!
  void updateIcon(QTreeWidgetItem *item, AGeoObject *obj);
  void menuActionFormStack(QList<QTreeWidgetItem *> selected);
  void markAsStackRefVolume(AGeoObject * obj);
  void updatePrototypeTreeGui();

  void menuActionAddNewObject(AGeoObject * ContObj, AGeoShape * shape);
  void menuActionCloneObject(AGeoObject * obj);
  void ShowObject(AGeoObject * obj);
  void ShowObjectRecursive(AGeoObject * obj);
  void ShowObjectOnly(AGeoObject * obj);
  void ShowAllInstances(AGeoObject * obj);
  void menuActionEnableDisable(AGeoObject * obj);
  void menuActionRemoveKeepContent(QTreeWidget * treeWidget);
  void menuActionRemoveHostedObjects(AGeoObject * obj);
  void menuActionRemoveWithContent(QTreeWidget * treeWidget);
  void menuActionAddNewComposite(AGeoObject * ContObj);
  void menuActionAddNewArray(AGeoObject * ContObj);
  void menuActionAddNewCircularArray(AGeoObject * ContObj);
  void menuActionAddNewGrid(AGeoObject * ContObj);
  void menuActionAddNewMonitor(AGeoObject * ContObj);
  void menuActionAddInstance(AGeoObject * ContObj, const QString & PrototypeName);
  void menuActionMakeItPrototype(const QList<QTreeWidgetItem *> & selected);
  void menuActionMoveProtoToWorld(AGeoObject * obj);
  void protoMenuEmptySelection(const QPoint & pos);

  QString makeScriptString_basicObject(AGeoObject *obj, bool bExpandMaterials, bool usePython) const;
  QString makeScriptString_slab(AGeoObject *obj, bool bExpandMaterials, int ident) const;
  QString makeScriptString_setCenterSlab(AGeoObject *obj) const;
  QString makeScriptString_arrayObject(AGeoObject *obj) const;
  QString makeScriptString_instanceObject(AGeoObject *obj, bool usePython) const;
  QString makeScriptString_prototypeObject(AGeoObject *obj) const;
  QString makeScriptString_monitorBaseObject(const AGeoObject *obj) const;
  QString makeScriptString_monitorConfig(const AGeoObject *obj) const;
  QString makeScriptString_stackObjectStart(AGeoObject *obj) const;
  QString makeScriptString_groupObjectStart(AGeoObject *obj) const;
  QString makeScriptString_stackObjectEnd(AGeoObject *obj) const;
  QString makeLinePropertiesString(AGeoObject *obj) const;
  QString makeScriptString_DisabledObject(AGeoObject *obj) const;

signals:
  void ObjectSelectionChanged(QString);
  void ProtoObjectSelectionChanged(QString);
  void RequestRebuildDetector();
  void RequestHighlightObject(QString name);
  void RequestFocusObject(QString name);
  void RequestShowObjectRecursive(QString name);
  void RequestShowAllInstances(QString name);
  void RequestNormalDetectorDraw();
  void RequestListOfParticles(QStringList & definedParticles);
  void RequestShowMonitor(const AGeoObject * mon);
  void RequestShowPrototypeList();
};

#endif // AGEOTREE_H
