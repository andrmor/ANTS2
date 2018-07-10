#ifndef AREMOTEWINDOW_H
#define AREMOTEWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QWidget>
#include <QFrame>

namespace Ui {
class ARemoteWindow;
}

class MainWindow;
class ARemoteServerRecord;
class AServerDelegate;
class AGridRunner;

class ARemoteWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ARemoteWindow(MainWindow *MW);
    ~ARemoteWindow();

private:
    MainWindow *MW;
    Ui::ARemoteWindow *ui;
    AGridRunner* GR;

    QVector<ARemoteServerRecord*> Records;
    QVector<AServerDelegate*> Delegates;

private slots:
    void onTextLogReceived(int index, const QString message);
    void onGuiUpdate();

    void onNameWasChanged();
    void on_pbStatus_clicked();
    void on_pbSimulate_clicked();

    void on_pbAdd_clicked();

private:
    void AddNewServer();

};

#endif // AREMOTEWINDOW_H