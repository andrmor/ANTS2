#ifndef AGEOBASEDELEGATE_H
#define AGEOBASEDELEGATE_H

#include <QObject>
#include <QString>

class AGeoObject;
class QPushButton;
class QHBoxLayout;

class AGeoBaseDelegate : public QObject
{
    Q_OBJECT

public:
    AGeoBaseDelegate(QWidget * ParentWidget);
    virtual ~AGeoBaseDelegate(){}

    virtual const QString getName() const = 0;
    virtual bool isValid(AGeoObject * obj) = 0;
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
