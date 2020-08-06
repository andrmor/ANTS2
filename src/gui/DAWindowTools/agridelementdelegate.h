#ifndef AGRIDELEMENTDELEGATE_H
#define AGRIDELEMENTDELEGATE_H

#include "ageobasedelegate.h"

#include <QString>

class QWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class AGeoObject;

class AGridElementDelegate : public AGeoBaseDelegate
{
    Q_OBJECT

public:
    AGridElementDelegate(QWidget * ParentWidget);

    const QString getName() const override;
    bool isValid(AGeoObject * obj) override;
    bool updateObject(AGeoObject * obj) const override;

private:
    QLineEdit * ledDX    = nullptr;
    QLineEdit * ledDY    = nullptr;
    QLineEdit * ledDZ    = nullptr;
    QComboBox * cobShape = nullptr;
    QLabel    * lSize1   = nullptr;
    QLabel    * lSize2   = nullptr;

    const AGeoObject * CurrentObject = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
    void onContentChanged();  //only to enter editing mode! Object update only on confirm button!
    void StartDialog();
    void onInstructionsForGridRequested();

signals:
    void RequestReshapeGrid(QString);

private:
    void updateVisibility();
};

#endif // AGRIDELEMENTDELEGATE_H
