#ifndef AGEOOBJECTDELEGATE_H
#define AGEOOBJECTDELEGATE_H

#include "ageobasedelegate.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <QStringList>

class AGeoObject;
class AGeoShape;

class TVector3;

class QWidget;
class QStringList;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLineEdit;
class QLabel;
class QCheckBox;
class QComboBox;
class QPlainTextEdit;
class QSpinBox;
class QPushButton;
class QLayout;
class QDialog;
class QListWidget;
class QTableWidget;
class AOneLineTextEdit;

class AGeoObjectDelegate : public AGeoBaseDelegate
{
    Q_OBJECT

public:
    AGeoObjectDelegate(const QStringList & materials, QWidget * ParentWidget = nullptr);
    virtual ~AGeoObjectDelegate();

    QString getName() const override;

    bool updateObject(AGeoObject * obj) const override;

    AGeoShape * ShapeCopy = nullptr;

private:
    QVBoxLayout * lMF = nullptr;      //main layout

    QWidget   * scaleWidget = nullptr;
    QLineEdit * ledScaleX   = nullptr;
    QLineEdit * ledScaleY   = nullptr;
    QLineEdit * ledScaleZ   = nullptr;

protected:
    const AGeoObject * CurrentObject = nullptr;

    QLabel    * labType = nullptr;
    QCheckBox * cbScale = nullptr;

    QString DelegateTypeName = "Object";
    QString ShapeHelp;

    QPushButton * pbTransform = nullptr;
    QPushButton * pbShapeInfo = nullptr;

    QLineEdit* leName;
    QComboBox* cobMat;
    QPlainTextEdit* pteShape;
    QWidget* PosOrient;
    QLabel* lMat;
    AOneLineTextEdit *ledNumX, *ledNumY, *ledNumZ;
    AOneLineTextEdit *ledStepX, *ledStepY, *ledStepZ;
    AOneLineTextEdit *ledX,   *ledY,     *ledZ;
    AOneLineTextEdit *ledPhi, *ledTheta, *ledPsi;

    QStringList ListOfShapesForTransform;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
    void onContentChanged();          // only to enter the editing mode! Object update is performed only on confirm button click!
    void onHelpRequested();           // dialog with AGeoShape list can be accessed here
    void onChangeShapePressed();
    void onScaleToggled();

protected slots:
    virtual void onLocalShapeParameterChange() {}

protected:
    void addLocalLayout(QLayout * lay);
    void updatePteShape(const QString & text);
    void updateScalingFactors();
    const AGeoShape * getBaseShapeOfObject(const AGeoObject *obj);
    void updateTypeLabel();
    void updateControlUI();

private:
    void rotate(TVector3 & v, double dPhi, double dTheta, double dPsi) const;
    void onShapeDialogActivated(QDialog * d, QListWidget * w);

signals:
    void RequestChangeShape(AGeoShape * newShape);
};

class AGeoBoxDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoBoxDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

    AOneLineTextEdit * ex = nullptr;
    AOneLineTextEdit * ey = nullptr;
    AOneLineTextEdit * ez = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AWorldDelegate : public AGeoBaseDelegate
{
    Q_OBJECT

public:
    AWorldDelegate(const QStringList & materials, QWidget * ParentWidget);

    QComboBox * cobMat = nullptr;
    QCheckBox * cbFixedSize = nullptr;

    AOneLineTextEdit * ledSizeXY = nullptr;
    AOneLineTextEdit * ledSizeZ  = nullptr;

    QString getName() const override;
    bool updateObject(AGeoObject * obj) const override;

public slots:
    void Update(const AGeoObject * obj) override;

};

class AGeoTubeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTubeDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

    AOneLineTextEdit * ei = nullptr;
    AOneLineTextEdit * eo = nullptr;
    AOneLineTextEdit * ez = nullptr;

protected:
    QGridLayout * gr = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoTubeSegDelegate : public AGeoTubeDelegate
{
    Q_OBJECT

public:
    AGeoTubeSegDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

    AOneLineTextEdit * ep1 = nullptr;
    AOneLineTextEdit * ep2 = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoTubeSegCutDelegate : public AGeoTubeSegDelegate
{
    Q_OBJECT

public:
    AGeoTubeSegCutDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * elnx = nullptr;
    QLineEdit * elny = nullptr;
    QLineEdit * elnz = nullptr;
    QLineEdit * eunx = nullptr;
    QLineEdit * euny = nullptr;
    QLineEdit * eunz = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class AGeoElTubeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoElTubeDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

    AOneLineTextEdit * ex = nullptr;
    AOneLineTextEdit * ey = nullptr;
    AOneLineTextEdit * ez = nullptr;

protected:
    QGridLayout * gr = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoParaDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoParaDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit * ex = nullptr;
    AOneLineTextEdit * ey = nullptr;
    AOneLineTextEdit * ez = nullptr;
    AOneLineTextEdit * ea = nullptr;
    AOneLineTextEdit * et = nullptr;
    AOneLineTextEdit * ep = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoSphereDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoSphereDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

    AOneLineTextEdit * eod = nullptr;
    AOneLineTextEdit * eid = nullptr;
    AOneLineTextEdit * et1 = nullptr;
    AOneLineTextEdit * et2 = nullptr;
    AOneLineTextEdit * ep1 = nullptr;
    AOneLineTextEdit * ep2 = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoConeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoConeDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

    AOneLineTextEdit * ez  = nullptr;
    AOneLineTextEdit * eli = nullptr;
    AOneLineTextEdit * elo = nullptr;
    AOneLineTextEdit * eui = nullptr;
    AOneLineTextEdit * euo = nullptr;

protected:
    QGridLayout * gr = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

protected slots:
};

class AGeoConeSegDelegate : public AGeoConeDelegate
{
    Q_OBJECT

public:
    AGeoConeSegDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit * ep1 = nullptr;
    AOneLineTextEdit * ep2 = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

protected slots:
    void onLocalShapeParameterChange() override;
};

class AGeoTrapXDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTrapXDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit * exl = nullptr;
    AOneLineTextEdit * exu = nullptr;
    AOneLineTextEdit * ey = nullptr;
    AOneLineTextEdit * ez = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoTrapXYDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTrapXYDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit * exl = nullptr;
    AOneLineTextEdit * exu = nullptr;
    AOneLineTextEdit * eyl = nullptr;
    AOneLineTextEdit * eyu = nullptr;
    AOneLineTextEdit * ez  = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class AGeoParaboloidDelegate : public AGeoObjectDelegate                      //paraboloiid
{
    Q_OBJECT

public:
    AGeoParaboloidDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit * el = nullptr;
    AOneLineTextEdit * eu = nullptr;
    AOneLineTextEdit * ez = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoTorusDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTorusDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit * ead = nullptr;
    AOneLineTextEdit * edi = nullptr;
    AOneLineTextEdit * edo = nullptr;
    AOneLineTextEdit * ep0 = nullptr;
    AOneLineTextEdit * epe = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

};

class AGeoPolygonDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoPolygonDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit  * en = nullptr;
    AOneLineTextEdit * edp = nullptr;
    AOneLineTextEdit * ez  = nullptr;
    AOneLineTextEdit * elo = nullptr;
    AOneLineTextEdit * eli = nullptr;
    AOneLineTextEdit * euo = nullptr;
    AOneLineTextEdit * eui = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class QTableWidget;
class AGeoPcon;
class AGeoPconDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoPconDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit * ep0 = nullptr;
    AOneLineTextEdit * epe = nullptr;

    QTableWidget * tab = nullptr;

protected:
    QVBoxLayout * lay = nullptr;
    void addOneLineTextEdits(int row);
signals:
    void reorderSections(int oldVisualIndex, int newVisualIndex);

public slots:
    void Update(const AGeoObject * obj) override;
    void updateTableW(AGeoPcon * pcon);
    void onCellEdited();
    void onAddAbove();
    void onAddBellow();

private slots:
    void onReorderSections(int /*logicalIndex*/, int oldVisualIndex, int newVisualIndex);

protected:
    const int rowHeight = 23;
};

class AGeoPgonDelegate : public AGeoPconDelegate
{
    Q_OBJECT

public:
    AGeoPgonDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters() override;

    AOneLineTextEdit  * eed = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:

};

class AGeoCompositeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoCompositeDelegate(const QStringList & materials, QWidget * parent);
    void finalizeLocalParameters();

    QPlainTextEdit * te = nullptr;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
};

struct AEditEdit
{
    QLineEdit * X = nullptr;
    QLineEdit * Y = nullptr;
};

class AGeoArb8Delegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoArb8Delegate(const QStringList & materials, QWidget * parent);

    QLineEdit * ez = nullptr;
    QVector< QVector<AEditEdit> > ve; //[0..1][0..3] - upper/lower, 4 points

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class AGeoArrayDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoArrayDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

public slots:
    void Update(const AGeoObject * obj) override;
};

class AGeoSetDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoSetDelegate(const QStringList & materials, QWidget * parent);

public slots:
    void Update(const AGeoObject * obj) override;
};

#endif // AGEOOBJECTDELEGATE_H
