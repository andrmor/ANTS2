#ifndef AGEOSLABDELEGATE_H
#define AGEOSLABDELEGATE_H

#include "ageobasedelegate.h"
#include "ageoobjectdelegate.h"

class QWidget;
class ASlabDelegate;
class QLabel;

class AGeoSlabDelegate : public AGeoBaseDelegate
{
  Q_OBJECT

public:
    AGeoSlabDelegate(const QStringList & definedMaterials, int State, QWidget * ParentWidget);

    QString getName() const override;
    bool updateObject(AGeoObject * obj) const override;

private:
    ASlabDelegate * SlabDel = nullptr;
    QLabel * labType = nullptr;

public slots:
    void Update(const AGeoObject* obj) override;

private slots:
    void onContentChanged();
};



class AGeoSlabDelegate_Box : public AGeoBoxDelegate
{
    Q_OBJECT
public:
    AGeoSlabDelegate_Box(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget);
    bool updateObject(AGeoObject * obj) const override;
private:
    int SlabModelState = 0;
public slots:
    void Update(const AGeoObject * obj) override;
};

class AGeoSlabDelegate_Tube : public AGeoTubeDelegate
{
    Q_OBJECT
public:
    AGeoSlabDelegate_Tube(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget);
    bool updateObject(AGeoObject * obj) const override;
private:
    int SlabModelState = 0;
public slots:
    void Update(const AGeoObject * obj) override;
};

class AGeoSlabDelegate_Poly : public AGeoPolygonDelegate
{
    Q_OBJECT
public:
    AGeoSlabDelegate_Poly(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget);
    bool updateObject(AGeoObject * obj) const override;
private:
    int SlabModelState = 0;
public slots:
    void Update(const AGeoObject * obj) override;
};

#endif // AGEOSLABDELEGATE_H
