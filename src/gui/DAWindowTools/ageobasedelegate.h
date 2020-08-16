#ifndef AGEOBASEDELEGATE_H
#define AGEOBASEDELEGATE_H

#include <QObject>
#include <QString>

class AGeoObject;
class QPushButton;
class QHBoxLayout;
class AOneLineTextEdit;

class AGeoBaseDelegate : public QObject
{
    Q_OBJECT

public:
    AGeoBaseDelegate(QWidget * ParentWidget);
    virtual ~AGeoBaseDelegate(){}

    virtual QString getName() const = 0;
    virtual void finalizeLocalParameters() {}             //activated in the beggining of "update object"
    virtual bool updateObject(AGeoObject * obj) const = 0;

public:
    QWidget * Widget = nullptr;

public slots:
    virtual void Update(const AGeoObject * obj) = 0;

protected:
    QPushButton * pbShow = nullptr;
    QPushButton * pbChangeAtt = nullptr;
    QPushButton * pbScriptLine = nullptr;

    QHBoxLayout * createBottomButtons();
    void          configureHighligherAndCompleter(AOneLineTextEdit * edit);

protected:
    QWidget * ParentWidget = nullptr;

signals:
    void ContentChanged();
    void RequestShow();
    void RequestChangeVisAttributes();
    void RequestScriptToClipboard();
    void RequestScriptRecursiveToClipboard();
};

#endif // AGEOBASEDELEGATE_H
