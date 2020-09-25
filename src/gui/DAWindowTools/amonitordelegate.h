#ifndef AMONITORDELEGATE_H
#define AMONITORDELEGATE_H

#include "ageobasedelegate.h"

class AMonitorDelegateForm;
class QStringList;

class AMonitorDelegate : public AGeoBaseDelegate
{
    Q_OBJECT

public:
    AMonitorDelegate(const QStringList & definedParticles, QWidget * ParentWidget);

    QString getName() const override;
    bool updateObject(AGeoObject * obj) const override;
    void Update(const AGeoObject * obj) override;

private:
    AMonitorDelegateForm * del = nullptr;

private slots:
    void onContentChanged();  //only to enter editing mode! Object update only on confirm button!

signals:
    void requestShowSensitiveFaces();
};

#endif // AMONITORDELEGATE_H
