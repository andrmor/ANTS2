#ifndef AREMOTEWINDOW_H
#define AREMOTEWINDOW_H

#include "aguiwindow.h"
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

class ARemoteWindow : public AGuiWindow
{
    Q_OBJECT

public:
    explicit ARemoteWindow(MainWindow *MW);
    ~ARemoteWindow();


public slots:
    void onBusy(bool flag);

private:
    MainWindow *MW;
    Ui::ARemoteWindow *ui;

    AGridRunner & GR;
    QVector<ARemoteServerRecord*> & Records;

    QVector<AServerDelegate*> Delegates;

private slots:
    void onTextLogReceived(int index, const QString message);
    void onStatusLogReceived(const QString message);
    void onGuiUpdate();
    void onNameWasChanged();
    void onUpdateSizeHint(AServerDelegate *d);

    //GUI user-driven actions
    void on_pbStatus_clicked();
    void on_pbSimulate_clicked();
    void on_pbReconstruct_clicked();
    void on_pbRateServers_clicked();
    void on_leiTimeout_editingFinished();
    void on_pbAdd_clicked();    
    void on_pbRemove_clicked();
    void on_pbAbort_clicked();

private:
    void Clear();
    void AddNewServerDelegate(ARemoteServerRecord* record);

};

#endif // AREMOTEWINDOW_H
