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
  void RequestListOfParticles(QStringList &definedParticles);
  void RequestShowMonitor(const AGeoObject* mon);
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
class AMonitorDelegateForm;
class QHBoxLayout;

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
  void onMonitorRequestsShowSensitiveDirection();

private slots:
  void onConfirmPressed();
  void onCancelPressed();

private:
  void rotate(TVector3* v, double dPhi, double dTheta, double dPsi);

  void getValuesFromNonSlabDelegates(AGeoObject *objMain);
  void confirmChangesForSlab();
  void confirmChangesForGridDelegate();
  void confirmChangesForMonitorDelegate();

  void exitEditingMode();
  QString getSuffix(AGeoObject *objCont);
  //void convertToLG(int UpperLower); //0 - upper, 1 - lower
  QLabel * addInfoLabel(QString text);
  AGeoObjectDelegate *createAndAddGeoObjectDelegate(AGeoObject *obj);
  ASlabDelegate *createAndAddSlabDelegate(AGeoObject *obj);
  AGridElementDelegate *createAndAddGridElementDelegate(AGeoObject *obj);
  AMonitorDelegate *createAndAddMonitorDelegate(AGeoObject *obj, QStringList particles);
  bool checkNonSlabObjectDelegateValidity(AGeoObject *obj);

signals:
  void showMonitor(const AGeoObject* mon);
};

class AGeoObjectDelegate : public QWidget
{
    Q_OBJECT

public:
    AGeoObjectDelegate(const QStringList & materials);
    virtual ~AGeoObjectDelegate(){}

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

    virtual const QString getLabel() const {return "";} // used to fill the label on top of the delegate

protected:
    const AGeoObject * CurrentObject = nullptr;
    QVBoxLayout * lMF = nullptr;      //main layout

public slots:
    virtual void Update(const AGeoObject * obj);

private slots:
    void onContentChanged();          // only to enter the editing mode! Object update is performed only on confirm button click!
    void onHelpRequested();           // dialog with AGeoShape list can be accessed here
    void onCursorPositionChanged();

protected slots:
    virtual void onLocalParameterChange() {}

signals:
    void ContentChanged();
};

class AGeoBoxDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoBoxDelegate(const QStringList & materials);

    QLineEdit * ex = nullptr;
    QLineEdit * ey = nullptr;
    QLineEdit * ez = nullptr;

    virtual const QString getLabel() const override {return "Box";}

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalParameterChange() override;
};

class AGeoTubeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTubeDelegate(const QStringList & materials);

    QLineEdit * ei = nullptr;
    QLineEdit * eo = nullptr;
    QLineEdit * ez = nullptr;

    virtual const QString getLabel() const override {return "Tube";}

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalParameterChange() override;
};

class AGeoParaDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoParaDelegate(const QStringList & materials);

    QLineEdit * ex = nullptr;
    QLineEdit * ey = nullptr;
    QLineEdit * ez = nullptr;
    QLineEdit * ea = nullptr;
    QLineEdit * et = nullptr;
    QLineEdit * ep = nullptr;

    virtual const QString getLabel() const override {return "Parallelepiped";}

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalParameterChange() override;
};

class AGeoSphereDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoSphereDelegate(const QStringList & materials);

    QLineEdit * eod = nullptr;
    QLineEdit * eid = nullptr;
    QLineEdit * et1 = nullptr;
    QLineEdit * et2 = nullptr;
    QLineEdit * ep1 = nullptr;
    QLineEdit * ep2 = nullptr;

    virtual const QString getLabel() const override {return "Sphere";}

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalParameterChange() override;
};

class AGeoConeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoConeDelegate(const QStringList & materials);

    QLineEdit * ez  = nullptr;
    QLineEdit * eli = nullptr;
    QLineEdit * elo = nullptr;
    QLineEdit * eui = nullptr;
    QLineEdit * euo = nullptr;

    virtual const QString getLabel() const override {return "Cone";}

public slots:
    virtual void Update(const AGeoObject * obj) override;

protected slots:
    void onLocalParameterChange() override;
};

// ---------------- Grid delegate ----------------

class AGridElementDelegate : public QWidget
{
  Q_OBJECT

public:
   AGridElementDelegate();

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

// ---------------- Monitor delegate ----------------

class AMonitorDelegate : public QWidget
{
  Q_OBJECT

public:
   AMonitorDelegate(const QStringList & definedParticles);

   QFrame* Widget;
   AMonitorDelegateForm* del;

   QString getName() const;
   void updateObject(AGeoObject* obj);

private:
   const AGeoObject* CurrentObject;

public slots:
  void Update(const AGeoObject* obj);

private slots:
  void onContentChanged();  //only to enter editing mode! Object update only on confirm button!

signals:
  void ContentChanged();
  void requestShowSensitiveFaces();
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
