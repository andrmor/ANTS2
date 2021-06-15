#ifndef AGEOSLABDELEGATE_H
#define AGEOSLABDELEGATE_H

#include "ageoobjectdelegate.h"

class QWidget;

class AGeoSlabDelegate_Box : public AGeoBoxDelegate
{
    Q_OBJECT
public:
    AGeoSlabDelegate_Box(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget);
    bool updateObject(AGeoObject * obj) const override;
    void Update(const AGeoObject * obj) override;
private:
    int SlabModelState = 0;
};

class AGeoSlabDelegate_Tube : public AGeoTubeDelegate
{
    Q_OBJECT
public:
    AGeoSlabDelegate_Tube(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget);
    bool updateObject(AGeoObject * obj) const override;
    void Update(const AGeoObject * obj) override;
private:
    int SlabModelState = 0;
};

class AGeoSlabDelegate_Poly : public AGeoPolygonDelegate
{
    Q_OBJECT
public:
    AGeoSlabDelegate_Poly(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget);
    bool updateObject(AGeoObject * obj) const override;
    void Update(const AGeoObject * obj) override;
private:
    int SlabModelState = 0;
};

#endif // AGEOSLABDELEGATE_H
