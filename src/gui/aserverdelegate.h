#ifndef ASERVERDELEGATE_H
#define ASERVERDELEGATE_H

#include <QString>
#include <QObject>
#include <QFrame>

class ARemoteServerRecord;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QLabel;
class QProgressBar;

class AServerDelegate : public QFrame
{
    Q_OBJECT
public:
    AServerDelegate(ARemoteServerRecord *modelRecord = 0);

    void           updateGui();

private:
    ARemoteServerRecord* modelRecord;

    QLabel*        labStatus;
    QLineEdit*     leName;
    QCheckBox*     cbEnabled;
    QLineEdit*     leIP;
    QSpinBox*      sbPort;
    //QLineEdit*     leiThreads;
    QLabel*        labThreads;
    QLabel*        labSpeedFactor;
    QProgressBar*  pbProgress;

private slots:
    void           updateModel();

private:
    void           setBackgroundGray(bool flag);
    void           setIcon(int option); // 0 is grey, 1 is green, 2 is red, 3 is yellow

signals:
    void           nameWasChanged();
    void           updateSizeHint(AServerDelegate*);

};

#endif // ASERVERDELEGATE_H
