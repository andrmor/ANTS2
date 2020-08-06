#include "ageoslabdelegate.h"
#include "slabdelegate.h"
#include "ageoobject.h"
#include "atypegeoobject.h"
#include "aslab.h"

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

const QString AGeoSlabDelegate::getName() const
{
    return SlabDel->leName->text();
}

bool AGeoSlabDelegate::isValid(AGeoObject * /*obj*/)
{
    return true;
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
