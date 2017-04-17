#ifndef LRFWINDOW_H
#define LRFWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QTimer>

class MainWindow;
class LRF2;
class PMsensorGroup;
class SensorLocalCache;
class QListWidgetItem;
class SensorLRFs;
class pms;

class EventsDataClass;

namespace Ui {
  class LRFwindow;
}

class LRFwindow : public QMainWindow
{
  Q_OBJECT
  
public:
    explicit LRFwindow(QWidget* parent, MainWindow *mw, EventsDataClass *eventsDataHub);
    ~LRFwindow();

    void SetProgress(int val);

    void onBusyOn();
    void onBusyOff();
    void SaveLRFDialog(QWidget *wid);
    void LoadLRFDialog(QWidget *wid);
    void showLRF();

    void writeToJson(QJsonObject &json) const;
    void loadJSON(QJsonObject &json);

    void updateLRFinfo(QJsonObject &json);
    void updateGroupsInfo();

    void updateIterationIndication();
    void SaveIteration(int iter, QString fileName);

    QString LastError;

    void saveLRFmakeSettingsToJson();

public slots:
  void LRF_ModuleReadySlot(bool ready);
  void on_pbMakeLRF_clicked();
  QString MakeLRFs(); //returns error string if error
  void DoMakeGroupLRF(int igroup);
  void onProgressReportReceived(int progress);  
  void onRequestGuiUpdate();

private slots:
  void contextMenuHistory(const QPoint &pos);
  void historyClickedTimeout();
  void on_pbStopLRFmake_clicked();
  void on_pbUpdateGUI_clicked();
  void on_pbSaveLRFs_clicked();
  void on_pbLoadLRFs_clicked();
  void on_pbShow2D_clicked();
  void on_sbPMnoButons_editingFinished();
  void on_sbPMnumberSpinButtons_valueChanged(int arg1);
  void on_sbPMnoButons_valueChanged(int arg1);
  void on_cb_data_2_clicked();
  void on_cb_diff_2_clicked();
  void on_pbShow1Ddistr_clicked();
  void on_pbShowSensorGroups_clicked();
  void on_pbUpdateHistory_clicked();
  void on_leCurrentLRFs_editingFinished();
  void on_pbClearHistory_clicked();
  void on_lwLRFhistory_itemClicked(QListWidgetItem *item);
  void on_pushButton_clicked();
  void on_pushButton_2_clicked();
  void on_pbClearTmpHistory_clicked();
  void on_pbSaveIterations_clicked();
  void on_pbExpand_clicked();
  void on_pbShrink_clicked();
  void on_pbRadialToText_clicked();
  void on_pbShowSensorGains_clicked();
  void on_pbAdjustOnlyGains_clicked();
  void on_led_compression_k_editingFinished();
  void on_cbUseGroupping_toggled(bool checked);
  void on_pbTableToAxial_clicked();
  void on_pbShowErrorVsRadius_clicked();
  void on_bConfigureFiltering_clicked();
  void on_pbUpdateLRFmakeJson_clicked();
  void on_pbShowRadialForXY_clicked();
  void on_actionPM_numbers_triggered();
  void on_actionPM_groups_triggered();
  void on_actionRelative_gain_triggered();
  void on_actionX_shift_triggered();
  void on_actionY_shift_triggered();
  void on_actionRotation_Flip_triggered();
  void on_pbLRFexplorer_clicked();
  void on_cb_data_clicked();
  void on_cb_diff_clicked();

  void on_pbAxial3DvsRandZ_clicked();

  void on_pbAxial3DvsZ_clicked();

protected:
    bool event(QEvent *event);

private:
    Ui::LRFwindow *ui;
    MainWindow *MW;
    SensorLRFs* SensLRF;
    pms* PMs;
    EventsDataClass *EventsDataHub;
    QTimer timer;

    bool StopSignal;  
    bool tmpIgnore; //temporary ignore update flag
    int secondIter; //second item in history selected - it will be shown on draw lrfs as blue
    bool fOneClick;

    void drawRadial();
    bool checkWatchdogs();
    void ShowTransform(int type);
};

#endif // LRFWINDOW_H
