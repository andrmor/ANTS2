#include "ageodelegatewidget.h"
#include "asandwich.h"
#include "ageotree.h"
#include "ageobasetreewidget.h"
#include "ageoobject.h"
#include "ageoshape.h"
#include "ageotype.h"
#include "ageobasedelegate.h"
#include "ageoobjectdelegate.h"
#include "ageoslabdelegate.h"
#include "agridelementdelegate.h"
#include "amonitordelegate.h"
#include "aslab.h"
#include "amessage.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QDebug>

AGeoDelegateWidget::AGeoDelegateWidget(ASandwich * Sandwich, AGeoTree * tw) :
  Sandwich(Sandwich), tw(tw)
{
  lMain = new QVBoxLayout(this);
  lMain->setContentsMargins(2,2,2,5);
  this->setLayout(lMain);

  //Scroll area in middle
  QScrollArea* sa = new QScrollArea(this);
  sa->setFrameShape(QFrame::Box);//NoFrame);
  sa->setContentsMargins(2,2,2,2);
  sa->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  sa->setWidgetResizable(true);
  sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QWidget* scrollAreaWidgetContents = new QWidget();
  scrollAreaWidgetContents->setGeometry(QRect(0, 0, 350, 200));

  ObjectLayout = new QVBoxLayout(scrollAreaWidgetContents);
  ObjectLayout->setContentsMargins(0,0,0,0);

  sa->setWidget(scrollAreaWidgetContents);
  lMain->addWidget(sa);

  frBottom = new QFrame();
  frBottom->setFrameShape(QFrame::StyledPanel);
  frBottom->setMinimumHeight(38);
  frBottom->setMaximumHeight(38);
  QPalette palette = frBottom->palette();
  palette.setColor( backgroundRole(), QColor( 255, 255, 255 ) );
  frBottom->setPalette( palette );
  frBottom->setAutoFillBackground( true );
  QHBoxLayout* lb = new QHBoxLayout();
  lb->setContentsMargins(0,0,0,0);
  frBottom->setLayout(lb);
    pbConfirm = new QPushButton("Confirm changes");
    pbConfirm->setMinimumHeight(25);
    connect(pbConfirm, SIGNAL(clicked()), this, SLOT(onConfirmPressed()));
    pbConfirm->setMaximumWidth(150);
    lb->addWidget(pbConfirm);
    pbCancel = new QPushButton("Cancel changes");
    connect(pbCancel, SIGNAL(clicked()), this, SLOT(onCancelPressed()));
    pbCancel->setMaximumWidth(150);
    pbCancel->setMinimumHeight(25);
    lb->addWidget(pbCancel);
  lMain->addWidget(frBottom);

  pbConfirm->setEnabled(false);
  pbCancel->setEnabled(false);

  fIgnoreSignals = false;
}

void AGeoDelegateWidget::ClearGui()
{
    //qDebug() << "AGeoWidget clear triggered (Delegate will be deleted)";
    fIgnoreSignals = true;

    while (ObjectLayout->count() > 0)
    {
        QLayoutItem * item = ObjectLayout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }

    delete GeoDelegate; GeoDelegate = nullptr;

    fIgnoreSignals = false;

    //if update triggered during editing
    exitEditingMode();
}

void AGeoDelegateWidget::UpdateGui()
{
    //qDebug() << "UpdateGui triggered for AGeoWidget--->->->->";
    ClearGui(); //deletes Delegate!

    if (!CurrentObject) return;

    pbConfirm->setEnabled(true);
    pbCancel->setEnabled(true);

    if (CurrentObject->ObjectType->isWorld())
        GeoDelegate = new AWorldDelegate(Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isSlab())        // SLAB or LIGHTGUIDE
        GeoDelegate = createAndAddSlabDelegate();
    else if (CurrentObject->ObjectType->isGridElement())
        GeoDelegate = createAndAddGridElementDelegate();
    else if (CurrentObject->ObjectType->isMonitor())
        GeoDelegate = createAndAddMonitorDelegate();
    else
        GeoDelegate = createAndAddGeoObjectDelegate();

    GeoDelegate->Update(CurrentObject);

    GeoDelegate->Widget->setEnabled(!CurrentObject->fLocked);
    connect(GeoDelegate, &AGeoBaseDelegate::ContentChanged,             this, &AGeoDelegateWidget::onStartEditing);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestChangeVisAttributes, this, &AGeoDelegateWidget::onRequestSetVisAttributes);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestShow,                this, &AGeoDelegateWidget::onRequestShowCurrentObject);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestScriptToClipboard,   this, &AGeoDelegateWidget::onRequestScriptLineToClipboard);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestScriptRecursiveToClipboard,   this, &AGeoDelegateWidget::onRequestScriptRecursiveToClipboard);

    ObjectLayout->addStretch();
    ObjectLayout->addWidget(GeoDelegate->Widget);
    ObjectLayout->addStretch();

    tw->LastShownObjectName = CurrentObject->Name;
}

AGeoBaseDelegate * AGeoDelegateWidget::createAndAddGeoObjectDelegate()
{
    AGeoObjectDelegate * Del;

    AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(CurrentObject->Shape);
    const QString shape = (scaled ? scaled->getBaseShapeType() : CurrentObject->Shape->getShapeType());

    if (CurrentObject->ObjectType->isArray())
        Del = new AGeoArrayDelegate(Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isInstance())
    {
        Del = new AGeoInstanceDelegate(Sandwich->Materials, this);
        connect((AGeoInstanceDelegate*)Del, &AGeoInstanceDelegate::RequestShowPrototype,        tw, &AGeoTree::onRequestShowPrototype);
        connect((AGeoInstanceDelegate*)Del, &AGeoInstanceDelegate::RequestIsValidPrototypeName, tw, &AGeoTree::onRequestIsValidPrototypeName);
    }
    else if (CurrentObject->ObjectType->isPrototype())
        Del = new AGeoPrototypeDelegate(Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isHandlingSet())
        Del = new AGeoSetDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoBBox")
        Del = new AGeoBoxDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTube")
        Del = new AGeoTubeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTubeSeg")
        Del = new AGeoTubeSegDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoCtub")
        Del = new AGeoTubeSegCutDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoEltu")
        Del = new AGeoElTubeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPara")
        Del = new AGeoParaDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoSphere")
        Del = new AGeoSphereDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTrd1")
        Del = new AGeoTrapXDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTrd2")
        Del = new AGeoTrapXYDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoCone")
        Del = new AGeoConeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoConeSeg")
        Del = new AGeoConeSegDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoParaboloid")
        Del = new AGeoParaboloidDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTorus")
        Del = new AGeoTorusDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPolygon")
        Del = new AGeoPolygonDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPcon")
        Del = new AGeoPconDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPgon")
        Del = new AGeoPgonDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoCompositeShape")
        Del = new AGeoCompositeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoArb8")
        Del = new AGeoArb8Delegate(Sandwich->Materials, this);
    else
        Del = new AGeoObjectDelegate(Sandwich->Materials, this);

    connect(Del, &AGeoObjectDelegate::RequestChangeShape,   this, &AGeoDelegateWidget::onRequestChangeShape);

    return Del;
}

AGeoBaseDelegate * AGeoDelegateWidget::createAndAddSlabDelegate()
{
    AGeoObjectDelegate * Del;
    ASlabModel * SlabModel = (static_cast<ATypeSlabObject*>(CurrentObject->ObjectType))->SlabModel;
    switch (SlabModel->XYrecord.shape)
    {
    default: qWarning() << "Unknown slab shape, assuming rectangular";
    case 0:
        Del = new AGeoSlabDelegate_Box(Sandwich->Materials, static_cast<int>(Sandwich->SandwichState), this); break;
    case 1:
        Del = new AGeoSlabDelegate_Tube(Sandwich->Materials, static_cast<int>(Sandwich->SandwichState), this); break;
    case 2:
        Del = new AGeoSlabDelegate_Poly(Sandwich->Materials, static_cast<int>(Sandwich->SandwichState), this); break;
    }
    connect(Del, &AGeoObjectDelegate::RequestChangeSlabShape, this, &AGeoDelegateWidget::onRequestChangeSlabShape);

    //Del = new AGeoSlabDelegate(tw->Sandwich->Materials, static_cast<int>(tw->Sandwich->SandwichState), this);
    return Del;
}

AGeoBaseDelegate * AGeoDelegateWidget::createAndAddGridElementDelegate()
{
    AGridElementDelegate * Del = new AGridElementDelegate(this);
    connect(Del, &AGridElementDelegate::RequestReshapeGrid, tw, &AGeoTree::onGridReshapeRequested);
    return Del;
}

AGeoBaseDelegate *AGeoDelegateWidget::createAndAddMonitorDelegate()
{
    QStringList particles;
    emit tw->RequestListOfParticles(particles);
    AMonitorDelegate* Del = new AMonitorDelegate(particles, this);
    connect(Del, &AMonitorDelegate::requestShowSensitiveFaces, this, &AGeoDelegateWidget::onMonitorRequestsShowSensitiveDirection);
    return Del;
}

void AGeoDelegateWidget::onObjectSelectionChanged(QString SelectedObject)
{
    if (fIgnoreSignals) return;

    //qDebug() << "Object selection changed! ->" << SelectedObject;

    CurrentObject = nullptr;
    ClearGui();
    if (SelectedObject.isEmpty()) return;

    AGeoObject * obj = Sandwich->World->findObjectByName(SelectedObject);
    //qDebug() << "Object for this name:" << obj;
    if (!obj) return;

    CurrentObject = obj;
    //qDebug() << "New current object:"<<CurrentObject->Name;
    UpdateGui();
    fEditingMode = false;
    //qDebug() << "OnObjectSelection procedure completed";
}

void AGeoDelegateWidget::onStartEditing()
{
  //qDebug() << "Start editing";
  if (fIgnoreSignals) return;
  if (!CurrentObject) return;

  if (!fEditingMode)
  {
      fEditingMode = true;
      tw->twGeoTree->setEnabled(false);
      tw->twPrototypes->setEnabled(false);
      QFont f = pbConfirm->font(); f.setBold(true); pbConfirm->setFont(f);
      pbConfirm->setStyleSheet("QPushButton {color: red;}");
      emit requestEnableGeoConstWidget(false);
  }
}

void AGeoDelegateWidget::onRequestChangeShape(AGeoShape * NewShape)
{
    if (!GeoDelegate) return;
    if (!CurrentObject) return;
    if (!NewShape) return;

    delete CurrentObject->Shape;
    CurrentObject->Shape = NewShape;
    if (!CurrentObject->ObjectType->isGrid()) CurrentObject->removeCompositeStructure();
    UpdateGui();
    onConfirmPressed();
}

void AGeoDelegateWidget::updateInstancesOnProtoNameChange(QString oldName, QString newName)
{
    QVector<AGeoObject*> vec;
    Sandwich->World->findAllInstancesRecursive(vec);

    for (AGeoObject * inst : vec)
    {
        ATypeInstanceObject * insType = static_cast<ATypeInstanceObject*>(inst->ObjectType);
        if (insType->PrototypeName == oldName)
            insType->PrototypeName = newName;
    }
}

void AGeoDelegateWidget::onRequestChangeSlabShape(int NewShape)
{
    if (!GeoDelegate) return;
    if (!CurrentObject) return;
    if (NewShape < 0 || NewShape > 2) return;
    if (!CurrentObject->ObjectType->isSlab()) return;

    ASlabModel * SlabModel = CurrentObject->getSlabModel();
    SlabModel->XYrecord.shape = NewShape;
    CurrentObject->UpdateFromSlabModel(SlabModel);

    exitEditingMode();
    QString name = CurrentObject->Name;
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

void AGeoDelegateWidget::onRequestShowCurrentObject()
{
    if (!CurrentObject) return;

    QString name = CurrentObject->Name;
    emit tw->RequestHighlightObject(name);
    tw->UpdateGui(name);
}

void AGeoDelegateWidget::onRequestScriptLineToClipboard()
{
    if (!CurrentObject) return;

    QString script;
    bool bNotRecursive = (CurrentObject->ObjectType->isSlab() || CurrentObject->ObjectType->isSingle() || CurrentObject->ObjectType->isComposite());
    emit requestBuildScript(CurrentObject, script, 0, false, !bNotRecursive, false);     // !*! the false may be temporary

    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoDelegateWidget::onRequestScriptRecursiveToClipboard()
{
    if (!CurrentObject) return;

    QString script;
    emit requestBuildScript(CurrentObject, script, 0, false, true, false);            // !*! the false may be temporary


    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoDelegateWidget::onRequestSetVisAttributes()
{
    if (!CurrentObject) return;
    tw->SetLineAttributes(CurrentObject);
}

QString AGeoDelegateWidget::getCurrentObjectName() const
{
    if (CurrentObject) return CurrentObject->Name;
    else return "";
}

void AGeoDelegateWidget::onMonitorRequestsShowSensitiveDirection()
{
    emit showMonitor(CurrentObject);
}

void AGeoDelegateWidget::exitEditingMode()
{
    fEditingMode = false;
    tw->twGeoTree->setEnabled(true);
    tw->twPrototypes->setEnabled(true);
    QFont f = pbConfirm->font(); f.setBold(false); pbConfirm->setFont(f);
    pbConfirm->setStyleSheet("QPushButton {color: black;}");
    pbConfirm->setEnabled(false);
    pbCancel->setEnabled(false);
    emit requestEnableGeoConstWidget(true);
}

void AGeoDelegateWidget::onConfirmPressed()
{
    if (!GeoDelegate)
    {
        qWarning() << "|||---Confirm triggered without active Delegate!";
        exitEditingMode();
        tw->UpdateGui();
        return;
    }

    const QString newName = GeoDelegate->getName();
    QString errorStr;
    if (newName != CurrentObject->Name && Sandwich->World->isNameExists(newName)) errorStr = QString("%1 name already exists").arg(newName);
    else if (newName.isEmpty()) errorStr = "Name cannot be empty";
    else if (newName.contains(QRegExp("\\s"))) errorStr = "Name cannot contain spaces";
    if (!errorStr.isEmpty())
    {
        QMessageBox::warning(this, "", errorStr);
        return;
    }

    const QString oldName = CurrentObject->Name;
    bool ok = GeoDelegate->updateObject(CurrentObject);
    if (!ok) return;

    if (CurrentObject->ObjectType->isPrototype() && oldName != newName)
        updateInstancesOnProtoNameChange(oldName, newName);

    exitEditingMode();
    QString name = CurrentObject->Name;
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

void AGeoDelegateWidget::onCancelPressed()
{
    exitEditingMode();
    tw->UpdateGui( (CurrentObject) ? CurrentObject->Name : "" );
}
