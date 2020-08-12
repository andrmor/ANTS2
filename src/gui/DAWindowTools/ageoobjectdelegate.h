#ifndef AGEOOBJECTDELEGATE_H
#define AGEOOBJECTDELEGATE_H

#include "ageobasedelegate.h"

#include <QObject>
#include <QString>
#include <QVector>

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

    const QString getName() const override;

    bool isValid(AGeoObject * obj) override;
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
    QLineEdit * ledSizeXY = nullptr;
    QLineEdit * ledSizeZ  = nullptr;

    const QString getName() const override;
    bool isValid(AGeoObject * obj) override;
    bool updateObject(AGeoObject * obj) const override;

public slots:
    void Update(const AGeoObject * obj) override;

private slots:
    void onContentChanged();

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
    virtual void Update(const AGeoObject * obj) override;

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
    virtual void Update(const AGeoObject * obj) override;

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
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class AGeoElTubeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoElTubeDelegate(const QStringList & materials, QWidget * parent);

    void finalizeLocalParameters();

    QLineEdit * ex = nullptr;
    QLineEdit * ey = nullptr;
    QLineEdit * ez = nullptr;

protected:
    QGridLayout * gr = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoParaDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoParaDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * ex = nullptr;
    QLineEdit * ey = nullptr;
    QLineEdit * ez = nullptr;
    QLineEdit * ea = nullptr;
    QLineEdit * et = nullptr;
    QLineEdit * ep = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
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
    virtual void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoConeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoConeDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * ez  = nullptr;
    QLineEdit * eli = nullptr;
    QLineEdit * elo = nullptr;
    QLineEdit * eui = nullptr;
    QLineEdit * euo = nullptr;

protected:
    QGridLayout * gr = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

protected slots:
    void onLocalShapeParameterChange() override;
};

class AGeoConeSegDelegate : public AGeoConeDelegate
{
    Q_OBJECT

public:
    AGeoConeSegDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * ep1 = nullptr;
    QLineEdit * ep2 = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

protected slots:
    void onLocalShapeParameterChange() override;
};

class AGeoTrapXDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTrapXDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * exl = nullptr;
    QLineEdit * exu = nullptr;
    QLineEdit * ey = nullptr;
    QLineEdit * ez = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class AGeoTrapXYDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTrapXYDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * exl = nullptr;
    QLineEdit * exu = nullptr;
    QLineEdit * eyl = nullptr;
    QLineEdit * eyu = nullptr;
    QLineEdit * ez  = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

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
    virtual void Update(const AGeoObject * obj) override;

private slots:
};

class AGeoTorusDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoTorusDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * ead = nullptr;
    QLineEdit * edi = nullptr;
    QLineEdit * edo = nullptr;
    QLineEdit * ep0 = nullptr;
    QLineEdit * epe = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class AGeoPolygonDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoPolygonDelegate(const QStringList & materials, QWidget * parent);

    QSpinBox  * sbn = nullptr;
    QLineEdit * edp = nullptr;
    QLineEdit * ez  = nullptr;
    QLineEdit * elo = nullptr;
    QLineEdit * eli = nullptr;
    QLineEdit * euo = nullptr;
    QLineEdit * eui = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;
};

class QTableWidget;
class AGeoPconDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoPconDelegate(const QStringList & materials, QWidget * parent);

    QLineEdit * ep0 = nullptr;
    QLineEdit * epe = nullptr;

    QTableWidget * tab = nullptr;

protected:
    QVBoxLayout * lay = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;

protected:
    const int rowHeight = 23;
};

class AGeoPgonDelegate : public AGeoPconDelegate
{
    Q_OBJECT

public:
    AGeoPgonDelegate(const QStringList & materials, QWidget * parent);

    QSpinBox  * sbn = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

private slots:
    void onLocalShapeParameterChange() override;

};

class AGeoCompositeDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoCompositeDelegate(const QStringList & materials, QWidget * parent);
    void finalizeLocalParameters();

    QPlainTextEdit * te = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) override;

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
    virtual void Update(const AGeoObject * obj) override;

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
    virtual void Update(const AGeoObject * obj) override;
};

class AGeoSetDelegate : public AGeoObjectDelegate
{
    Q_OBJECT

public:
    AGeoSetDelegate(const QStringList & materials, QWidget * parent);

public slots:
    virtual void Update(const AGeoObject * obj) override;
};

// ---------------------------------------------------

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegExp>

class QTextDocument;

class AShapeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    AShapeHighlighter(QTextDocument * parent = nullptr);
protected:
    void highlightBlock(const QString & text);
private:
    struct HighlightingRule
      {
        QRegExp pattern;
        QTextCharFormat format;
      };
    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat ShapeFormat;
};

#endif // AGEOOBJECTDELEGATE_H
