#ifndef AREMOTEWINDOW_H
#define AREMOTEWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QWidget>
#include <QFrame>

namespace Ui {
class ARemoteWindow;
}

class AServerDelegate;
class QLineEdit;
class QSpinBox;
class QProgressBar;

class ARemoteWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ARemoteWindow(QWidget *parent = 0);
    ~ARemoteWindow();

private:
    Ui::ARemoteWindow *ui;
    QVector<AServerDelegate*> Delegates;

private slots:
    void onTextLogReceived(int index, const QString message);

    void onNameWasChanged();
    void on_pbStatus_clicked();

private:
    void AddNewServer();
};

class AServerDelegate : public QFrame//QWidget
{
    Q_OBJECT
public:
    AServerDelegate(const QString& ip, int port);

    const QString  getName() const;
    const QString  getIP() const;
    int            getPort() const;

    void           setIP(const QString& ip);
    void           setPort(int port);

    void           setThreads(int threads);
    void           setProgress(int progress);
    void           setProgressVisible(bool flag);

private:
    QLineEdit*     leName;
    QLineEdit*     leIP;
    QSpinBox*      sbPort;
    QLineEdit*     leThreads;
    QProgressBar*  pbProgress;

signals:
    void           nameWasChanged();

};

#endif // AREMOTEWINDOW_H
