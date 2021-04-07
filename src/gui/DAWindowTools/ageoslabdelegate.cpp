#include "ageoslabdelegate.h"
#include "ageoobject.h"
#include "ageoshape.h"
#include "ageotype.h"
#include "aslab.h"
#include "amessage.h"
#include "aonelinetextedit.h"
#include "ageoconsts.h"

#include <QDebug>
#include <QWidget>
#include <QLabel>

AGeoSlabDelegate_Box::AGeoSlabDelegate_Box(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget) :
    AGeoBoxDelegate(definedMaterials, ParentWidget), SlabModelState(SlabModelState)
{
    initSlabDelegate(SlabModelState);
    if (SlabModelState == 0)
    {
        ex->setEnabled(false);
        ey->setEnabled(false);
    }
}

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
        //[[fallthrough]];
    case 1: //update dx dy
        SlabModel.XYrecord.strSize1 = ex->text();
        ok = ok && GC.updateParameter(ErrorStr, SlabModel.XYrecord.strSize1, SlabModel.XYrecord.size1, true, true, false);
        SlabModel.XYrecord.strSize2 = ey->text();
        ok = ok && GC.updateParameter(ErrorStr, SlabModel.XYrecord.strSize2, SlabModel.XYrecord.size2, true, true, false);
        //[[fallthrough]];
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

AGeoSlabDelegate_Tube::AGeoSlabDelegate_Tube(const QStringList &definedMaterials, int SlabModelState, QWidget *ParentWidget) :
    AGeoTubeDelegate(definedMaterials, ParentWidget), SlabModelState(SlabModelState)
{
    initSlabDelegate(SlabModelState);
    ei->setEnabled(false);
    if (SlabModelState == 0) eo->setEnabled(false);

    ei->   setVisible(false);
    labIm->setVisible(false);
    labIu->setVisible(false);
}

bool AGeoSlabDelegate_Tube::updateObject(AGeoObject *obj) const
{
    ATypeSlabObject * slab = dynamic_cast<ATypeSlabObject*>(obj->ObjectType);
    if (!slab)
    {
        qWarning() << "This is not a slab!";
        return false;
    }

    bool ok = AGeoTubeDelegate::updateObject(obj);
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
        //[[fallthrough]];
    case 1: //update dx dy
        SlabModel.XYrecord.strSize1 = eo->text();
        ok = ok && GC.updateParameter(ErrorStr, SlabModel.XYrecord.strSize1, SlabModel.XYrecord.size1, true, true, false);
        //[[fallthrough]];
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

void AGeoSlabDelegate_Tube::Update(const AGeoObject *obj)
{
    AGeoTubeDelegate::Update(obj);
    labType->setText( CurrentObject->ObjectType->isLightguide() ? "Lightguide, round slab" : "Round slab" );
}

AGeoSlabDelegate_Poly::AGeoSlabDelegate_Poly(const QStringList &definedMaterials, int SlabModelState, QWidget *ParentWidget) :
    AGeoPolygonDelegate(definedMaterials, ParentWidget), SlabModelState(SlabModelState)
{
    initSlabDelegate(SlabModelState);
    if (SlabModelState == 0)
        elo->setEnabled(false);

    if (SlabModelState != 2) en->setEnabled(false);

    for (QLabel * lab : {labLI, labUO, labUI, labA, labLIu, labUOu, labUIu, labAu}) lab->setVisible(false);
    for (AOneLineTextEdit * lab : {euo, edp, eli, eui}) lab->setVisible(false);

    labLO->setText("Diameter of incircle:");
}

bool AGeoSlabDelegate_Poly::updateObject(AGeoObject *obj) const
{
    ATypeSlabObject * slab = dynamic_cast<ATypeSlabObject*>(obj->ObjectType);
    if (!slab)
    {
        qWarning() << "This is not a slab!";
        return false;
    }

    bool ok = AGeoPolygonDelegate::updateObject(obj);
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
        ok = GC.updateParameter(ErrorStr, SlabModel.XYrecord.strAngle, SlabModel.XYrecord.angle, false, false, false);
        if (ok)
        {
            SlabModel.XYrecord.strSides = en->text();
            double sides;
            ok = GC.updateParameter(ErrorStr, SlabModel.XYrecord.strSides, sides, true, true, false);
            if (sides < 3)
            {
                ErrorStr = "Number of sides should be at least 3";
                ok = false;
            }
            else SlabModel.XYrecord.sides = sides;
        }
        //[[fallthrough]];
    case 1: //update dx dy
        SlabModel.XYrecord.strSize1 = elo->text();
        ok = ok && GC.updateParameter(ErrorStr, SlabModel.XYrecord.strSize1, SlabModel.XYrecord.size1, true, true, false);
        //[[fallthrough]];
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

void AGeoSlabDelegate_Poly::Update(const AGeoObject *obj)
{
    AGeoPolygonDelegate::Update(obj);
    labType->setText( CurrentObject->ObjectType->isLightguide() ? "Lightguide, polygon slab" : "Polygon slab" );
}
