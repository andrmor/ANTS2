#ifndef AGEOSLABDELEGATE_H
#define AGEOSLABDELEGATE_H

#include "ageobasedelegate.h"

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



#include "ageoobjectdelegate.h"

class ASlabModel;

class AGeoSlabDelegate_Box : public AGeoBoxDelegate
{
    Q_OBJECT

public:
    AGeoSlabDelegate_Box(const QStringList & definedMaterials, int SlabModelState, QWidget * ParentWidget);

    bool updateObject(AGeoObject * obj) const override;

private:
    ASlabModel * SlabModel = nullptr;
    int SlabModelState = 0;

public slots:
    void Update(const AGeoObject * obj) override;
};

#endif // AGEOSLABDELEGATE_H
