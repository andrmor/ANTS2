#ifndef AREMOTEWINDOW_H
#define AREMOTEWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QWidget>
#include <QFrame>

namespace Ui {
class ARemoteWindow;
}

class ARemoteServerRecord;
class AServerDelegate;

class ARemoteWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ARemoteWindow(QWidget *parent = 0);
    ~ARemoteWindow();

private:
    Ui::ARemoteWindow *ui;

    QVector<ARemoteServerRecord*> Records;
    QVector<AServerDelegate*> Delegates;

private slots:
    void onTextLogReceived(int index, const QString message);
    void onGuiUpdate();

    void onNameWasChanged();
    void on_pbStatus_clicked();

    void on_pbAdd_clicked();

private:
    void AddNewServer();

};

#endif // AREMOTEWINDOW_H
