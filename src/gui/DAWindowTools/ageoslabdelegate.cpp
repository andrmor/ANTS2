#include "ageoslabdelegate.h"
#include "slabdelegate.h"
#include "ageoobject.h"
#include "atypegeoobject.h"
#include "aslab.h"
#include "amessage.h"

#include <QDebug>
#include <QWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFont>
#include <QPalette>
#include <QComboBox>
#include <QCheckBox>

AGeoSlabDelegate::AGeoSlabDelegate(const QStringList & definedMaterials, int State, QWidget * ParentWidget)
    : AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;
    Widget->setContextMenuPolicy(Qt::CustomContextMenu);

    QPalette palette = frMainFrame->palette();
    palette.setColor( Widget->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette(palette);
    frMainFrame->setAutoFillBackground( true );
    QVBoxLayout * lMF = new QVBoxLayout();
        lMF->setContentsMargins(0,5,0,5);

        //object type
        labType = new QLabel("Slab");
        labType->setAlignment(Qt::AlignCenter);
        QFont font = labType->font();
        font.setBold(true);
        labType->setFont(font);
        lMF->addWidget(labType);

        SlabDel = new ASlabDelegate();
        SlabDel->leName->setMinimumWidth(50);

        ASlabXYDelegate::ShowStates XYDelState = static_cast<ASlabXYDelegate::ShowStates>(State);
        switch (XYDelState)
        {
        case (0): //ASandwich::CommonShapeSize
            SlabDel->XYdelegate->SetShowState(ASlabXYDelegate::ShowNothing); break;
        case (1): //ASandwich::CommonShape
            SlabDel->XYdelegate->SetShowState(ASlabXYDelegate::ShowSize); break;
        case (2): //ASandwich::Individual
            SlabDel->XYdelegate->SetShowState(ASlabXYDelegate::ShowAll); break;
        default:
            qWarning() << "Unknown DetectorSandwich state!"; break;
        }

        SlabDel->frMain->setPalette( palette );
        SlabDel->frMain->setAutoFillBackground( true );
        SlabDel->frMain->setMaximumHeight(80);
        SlabDel->frMain->setFrameShape(QFrame::NoFrame);
        connect(SlabDel->XYdelegate, SIGNAL(ContentChanged()), SlabDel->XYdelegate, SLOT(updateComponentVisibility()));
        connect(SlabDel, &ASlabDelegate::ContentChanged, this, &AGeoSlabDelegate::onContentChanged);

        SlabDel->comMaterial->addItems(definedMaterials);

        //SlabDel->frMain->setMinimumHeight(65);

     lMF->addWidget(SlabDel->frMain);

     // bottom line buttons
     QHBoxLayout * abl = createBottomButtons();
     abl->setContentsMargins(5,0,5,0);
     lMF->addLayout(abl);

     Widget->setLayout(lMF);
}

QString AGeoSlabDelegate::getName() const
{
    return SlabDel->leName->text();
}

bool AGeoSlabDelegate::updateObject(AGeoObject * obj) const
{
    ASlabModel * model = obj->getSlabModel();

    obj->Name = getName();
    bool fCenter = model->fCenter;
    SlabDel->UpdateModel(model);
    model->fCenter = fCenter; //delegate does not remember center status
    return true;
}

void AGeoSlabDelegate::Update(const AGeoObject *obj)
{
    QString str = "Slab";
    if (obj->ObjectType->isLightguide())
    {
        if (obj->ObjectType->isUpperLightguide()) str += ", Upper lightguide";
        else if (obj->ObjectType->isLowerLightguide()) str += ", Lower lightguide";
        else str = "Lightguide error!";
    }
    labType->setText(str);

    ASlabModel* SlabModel = (static_cast<ATypeSlabObject*>(obj->ObjectType))->SlabModel;
    SlabDel->UpdateGui(*SlabModel);

    SlabDel->frCenterZ->setVisible(false);
    SlabDel->fCenter = false;
    SlabDel->frColor->setVisible(false);
    SlabDel->cbOnOff->setEnabled(!SlabModel->fCenter);
}

void AGeoSlabDelegate::onContentChanged()
{
    emit ContentChanged();
}

// ------------------------------------------- BOX ------------------------------------

#include "aonelinetextedit.h"
#include "ageoshape.h"

#include <QPushButton>

AGeoSlabDelegate_Box::AGeoSlabDelegate_Box(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget) :
    AGeoBoxDelegate(definedMaterials, ParentWidget), SlabModelState(SlabModelState)
{
    switch (SlabModelState) // supposed to fall through!
    {
    case 0 :
        ex->setEnabled(false);
        ey->setEnabled(false);
    case 1 :
        ledPsi->setEnabled(false);
    case 2 :;
    default:;
    }

    ledX->setEnabled(false);
    ledY->setEnabled(false);
    ledZ->setEnabled(false);

    ledPhi->setEnabled(false);
    ledTheta->setEnabled(false);

    cbScale->setVisible(false);

    if (SlabModelState == 2) ListOfShapesForTransform = QStringList({"Rectangular slab", "Round slab", "Polygon slab"});
    else pbTransform->setEnabled(false);
}

#include "ageoconsts.h"
bool AGeoSlabDelegate_Box::updateObject(AGeoObject * obj) const
{
    ATypeSlabObject * slab = dynamic_cast<ATypeSlabObject*>(obj->ObjectType);
    if (!slab)
    {
        qWarning() << "This is not a slab!";
        return false;
    }

    bool ok = AGeoBoxDelegate::updateObject(obj);
    if (!ok) return false;

    ASlabModel SlabModel = *slab->SlabModel;

    SlabModel.material = obj->Material;
    SlabModel.name     = obj->Name;

    const AGeoConsts & GC = AGeoConsts::getConstInstance();
    QString ErrorStr;
    switch (SlabModelState)  // supposed to fall through!
    {
    default: qWarning() << "Unknown slab shape, assuming rectangular";
    case 2: //update psi
        SlabModel.XYrecord.strAngle = ledPsi->text();
        ok =       GC.updateParameter(ErrorStr, SlabModel.XYrecord.strAngle, SlabModel.XYrecord.angle, false, false, false);
    case 1: //update dx dy
        SlabModel.XYrecord.strSize1 = ex->text();
        ok = ok && GC.updateParameter(ErrorStr, SlabModel.XYrecord.strSize1, SlabModel.XYrecord.size1, true, true, false);
        SlabModel.XYrecord.strSize2 = ey->text();
        ok = ok && GC.updateParameter(ErrorStr, SlabModel.XYrecord.strSize2, SlabModel.XYrecord.size2, true, true, false);
    case 0: //update dz
        SlabModel.strHeight = ez->text();
        ok = ok && GC.updateParameter(ErrorStr, SlabModel.strHeight, SlabModel.height, true, true, false);
    }

    if (!ok)
    {
        message(ErrorStr, this->ParentWidget);
        return false;
    }

    *(slab->SlabModel) = SlabModel;
    return true;
}

void AGeoSlabDelegate_Box::Update(const AGeoObject * obj)
{
    AGeoBoxDelegate::Update(obj);

    labType->setText( CurrentObject->ObjectType->isLightguide() ? "Lightguide, rectangular slab" : "Rectangular slab" );
}
