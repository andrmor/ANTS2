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

    void           setThreads(int threads);
    void           setProgress(int progress);
    void           setProgressVisible(bool flag);

    void           updateGui();

private:
    ARemoteServerRecord* modelRecord;

    QLabel*        labStatus;
    QLineEdit*     leName;
    QCheckBox*     cbEnabled;
    QLineEdit*     leIP;
    QSpinBox*      sbPort;
    QLineEdit*     leiThreads;
    QProgressBar*  pbProgress;

private slots:
    void           updateModel();

private:
    void           setBackgroundGray(bool flag);
    void           setIcon(int option); // 1 is grey, 2 is green, 3+ is red

signals:
    void           nameWasChanged();

};

#endif // ASERVERDELEGATE_H
