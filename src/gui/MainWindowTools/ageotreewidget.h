#ifndef AGEOTREEWIDGET_H
#define AGEOTREEWIDGET_H

#include <QTreeWidget>
#include <QSyntaxHighlighter>

class AGeoObject;
class AGeoWidget;
class ASandwich;

class AGeoTreeWidget : public QTreeWidget
{
  Q_OBJECT

public:
  AGeoTreeWidget(ASandwich* Sandwich);
  AGeoWidget* GetEditWidget() {return EditWidget;}

  ASandwich *Sandwich;

  void SelectObjects(QStringList ObjectNames);

public slots:
  void UpdateGui(QString selected = "");
  void onGridReshapeRequested(QString objName);

private slots:
  void onItemSelectionChanged();
  void customMenuRequested(const QPoint &pos);  // CONTEXT MENU
  void onItemClicked(); //only to return to normal geo view mode!

  void onItemExpanded(QTreeWidgetItem* item);
  void onItemCollapsed(QTreeWidgetItem* item);

protected:
  void dropEvent(QDropEvent *event);
  void dragEnterEvent(QDragEnterEvent* event);
//  void dragMoveEvent(QDragMoveEvent* event);

private:  
  AGeoObject *World;
  AGeoWidget *EditWidget;

  //base images for icons
  QImage Lock;
  QImage GroupStart, GroupMid, GroupEnd;
  QImage StackStart, StackMid, StackEnd;

  QColor BackgroundColor;
  bool fSpecialGeoViewMode;

  void populateTreeWidget(QTreeWidgetItem *parent, AGeoObject *Container, bool fDisabled = false);
  void updateExpandState(QTreeWidgetItem* item); //recursive!
  void updateIcon(QTreeWidgetItem *item, AGeoObject *obj);
  void formSet(QList<QTreeWidgetItem *> selected, int option);
  void addLightguide(bool upper);

  void menuActionAddNewObject(QString ContainerName);
  void menuActionCopyObject(QString ObjToCopyName);
  void SetLineAttributes(QString ObjectName);
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
signals:
  void ObjectSelectionChanged(const QString); // should be fired with empty string if selection does not contain a single item  
  void RequestRebuildDetector();
  void RequestHighlightObject(QString name);
  void RequestShowObjectRecursive(QString name);
  void RequestNormalDetectorDraw();
};

// GeoWidget
class QLineEdit;
class QPlainTextEdit;
class QComboBox;
class QSpinBox;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class AGeoObjectDelegate;
class AGridElementDelegate;
class AMonitorDelegate;
class QLabel;
class ASlabDelegate;
class ATypeCompositeObject;
class TVector3;

class AGeoWidget : public QWidget
{
  Q_OBJECT

public:
  AGeoWidget(AGeoObject* World, AGeoTreeWidget* tw);
  //destructor does not delete Widget - it is handled by layout

private:
  AGeoObject* World;
  AGeoTreeWidget* tw;

  AGeoObject* CurrentObject;

  AGeoObjectDelegate* GeoObjectDelegate;
  ASlabDelegate* SlabDelegate;
  AGridElementDelegate* GridDelegate;
  AMonitorDelegate* MonitorDelegate;

  QVBoxLayout *lMain;
  QVBoxLayout *ObjectLayout;
  QFrame      *frBottom;
  QPushButton *pbConfirm, *pbCancel;

  bool fIgnoreSignals;
  bool fEditingMode;

  void ClearGui();
  void UpdateGui();

public slots:
  void onObjectSelectionChanged(const QString SelectedObject); //starts GUI update
  void onStartEditing();
  void OnCustomContextMenuTriggered_forMainObject(QPoint pos);

private slots:
  void onConfirmPressed();
  void onCancelPressed();

private:
  void rotate(TVector3* v, double dPhi, double dTheta, double dPsi);
  void confirmChangesForSlab();
  void confirmChangesForGridDelegate();

  void exitEditingMode();
  QString getSuffix(AGeoObject *objCont);
  //void convertToLG(int UpperLower); //0 - upper, 1 - lower
  void addInfoLabel(QString text);
  AGeoObjectDelegate *createAndAddGeoObjectDelegate(AGeoObject *obj);
  ASlabDelegate *createAndAddSlabDelegate(AGeoObject *obj);
  AGridElementDelegate *createAndAddGridElementDelegate(AGeoObject *obj);
  AMonitorDelegate *createAndAddMonitorDelegate(AGeoObject *obj);

  bool checkNonSlabObjectDelegateValidity(AGeoObject *obj);
  void getValuesFromNonSlabDelegates(AGeoObject *objMain);
};


class AGeoObjectDelegate : public QWidget
{
  Q_OBJECT

public:
  AGeoObjectDelegate(QString name, QStringList materials);

  QFrame* Widget;

  QFrame* frMainFrame;  
  QLineEdit* leName;
  QComboBox* cobMat;
  QPlainTextEdit* pteShape;
  QWidget* PosOrient;
  QWidget* ArrayWid;
  QPushButton* pbHelp;
  QLabel* lMat;

  QSpinBox *sbNumX, *sbNumY, *sbNumZ;
  QLineEdit *ledStepX, *ledStepY, *ledStepZ;

  QLineEdit *ledX, *ledY, *ledZ;
  QLineEdit *ledPhi, *ledTheta, *ledPsi;

private:
  const AGeoObject* CurrentObject;

public slots:
  void Update(const AGeoObject* obj);

private slots:
  void onContentChanged();  //only to enter editing mode! Object update only on confirm button!
  void onHelpRequested();   //AGeoShape list is here!!!
  void onCursorPositionChanged();

signals:
  void ContentChanged();

};

class AGridElementDelegate : public QWidget
{
  Q_OBJECT

public:
   AGridElementDelegate(QString name);

   QFrame* Widget;

   QFrame* frMainFrame;
   QLineEdit *ledDX, *ledDY, *ledDZ;
   QComboBox *cobShape;
   QLabel *lSize1, *lSize2;

private:
   const AGeoObject* CurrentObject;

   void updateVisibility();

public slots:
  void Update(const AGeoObject* obj);

private slots:
  void onContentChanged();  //only to enter editing mode! Object update only on confirm button!
  void StartDialog();
  void onInstructionsForGridRequested();

signals:
  void ContentChanged();
  void RequestReshapeGrid(QString);
};

class AMonitorDelegate : public QWidget
{
  Q_OBJECT

public:
   AMonitorDelegate(QString name);

   QFrame* Widget;

   QFrame    *frMainFrame;
   QLineEdit *leName;
   QLineEdit *ledDX, *ledDY;
   QComboBox *cobShape;
   QLabel    *lSize1, *lSize2;

   QLineEdit *ledX, *ledY, *ledZ;
   QLineEdit *ledPhi, *ledTheta, *ledPsi;

private:
   const AGeoObject* CurrentObject;

public slots:
  void Update(const AGeoObject* obj);

private slots:
  void onContentChanged();  //only to enter editing mode! Object update only on confirm button!
  void updateVisibility();

signals:
  void ContentChanged();
};

class AShapeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    AShapeHighlighter(QTextDocument *parent = 0);
protected:
    void highlightBlock(const QString &text);
private:
    struct HighlightingRule
      {
        QRegExp pattern;
        QTextCharFormat format;
      };
    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat ShapeFormat;
};

#endif // AGEOTREEWIDGET_H
