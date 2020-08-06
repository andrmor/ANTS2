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

    const QString getName() const override;
    bool isValid(AGeoObject * obj) override;
    bool updateObject(AGeoObject * obj) const override;

private:
    ASlabDelegate * SlabDel = nullptr;
    QLabel * labType = nullptr;

public slots:
    void Update(const AGeoObject* obj) override;

private slots:
    void onContentChanged();
};

#endif // AGEOSLABDELEGATE_H
