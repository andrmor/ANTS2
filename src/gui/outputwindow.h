#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include "aguiwindow.h"
#include <QGraphicsScene>

class MainWindow;
class myQGraphicsView;
class QStandardItemModel;
struct AReconRecord;
class EventsDataClass;
class DynamicPassivesHandler;
class QTreeWidgetItem;
class AParticleTrackingRecord;

namespace Ui {
class OutputWindow;
}

class OutputWindow : public AGuiWindow
{
    Q_OBJECT
    
public:
    explicit OutputWindow(QWidget* parent, MainWindow *mw, EventsDataClass *eventsDataHub);
    ~OutputWindow();

    QVector< QBitArray > SiPMpixels; //[PM#] [time] [pixY] [pixX] - to report only!

    void PMnumChanged();
    void SetCurrentEvent(int iev);

    void OutTextClear();
    void OutText(const QString & str);

    void SetTab(int index);

    void RefreshData();
    void ResetViewport(); //auto-size and center viewing area
    void InitWindow();

    void ShowGeneratedPhotonsLog();
    void ShowOneEventLog(int iev);    
    void ShowPhotonProcessesLog();
    void ShowPhotonLossLog();

    void UpdateMaterials();
    void UpdateParticles();

protected:
    void resizeEvent(QResizeEvent *event);
    
private slots:
    void on_pbShowPMtime_clicked();
    void on_sbPMnumberToShowTime_valueChanged(int arg1);
    void on_pbSiPMpixels_clicked();
    void on_sbTimeBin_valueChanged(int arg1);
    void on_pbRefreshViz_clicked();
    void on_tabwinDiagnose_currentChanged(int index);
    void on_pbWaveSpectrum_clicked();
    void on_pbTimeSpectrum_clicked();
    void on_pbAngleSpectrum_clicked();
    void on_pbNumTransitionsSpectrum_clicked();
    void on_sbEvent_valueChanged(int arg1);
    void on_pbResetViewport_clicked();
    void on_pbAverageSignals_clicked();
    void on_pbShowPhotonLog_clicked();
    void on_pbShowSumSignals_clicked();
    void on_pbShowSignals_clicked();
    void on_pbShowPhotonProcessesLog_clicked();
    void on_pbShowPhotonLossLog_clicked();    
    void on_pbNextEvent_clicked();
    void on_pbPreviousEvent_clicked();

    void on_tabwinDiagnose_tabBarClicked(int index);
    void RefreshPMhitsTable();

    void on_pbMonitorShowXY_clicked();
    void on_pbMonitorShowTime_clicked();
    void on_pbShowProperties_clicked();
    void on_pbMonitorShowAngle_clicked();
    void on_pbMonitorShowWave_clicked();
    void on_pbMonitorShowEnergy_clicked();
    void on_cobMonitor_activated(int index);
    void on_pbShowAverageOverAll_clicked();
    void on_pbShowWavelength_clicked();
    void on_pbPTHistRequest_clicked();
    void on_cbPTHistOnlyPrim_clicked(bool checked);
    void on_cbPTHistOnlySec_clicked(bool checked);
    void on_cobPTHistVolRequestWhat_currentIndexChanged(int index);
    void on_twPTHistType_currentChanged(int index);
    void on_cbPTHistBordVs_toggled(bool checked);
    void on_cbPTHistBordAndVs_toggled(bool checked);
    void on_cbPTHistBordAsStat_toggled(bool checked);
    void on_pbEventView_ShowTree_clicked();
    void on_pbEVgeo_clicked();

    void on_sbEVexpansionLevel_valueChanged(int arg1);

private:
    Ui::OutputWindow *ui;
    MainWindow* MW;
    EventsDataClass *EventsDataHub;

    QGraphicsScene *scene;
    QGraphicsScene *scaleScene;
    myQGraphicsView *gvOut;
    QStandardItemModel *modelPMhits;
    double GVscale;
    QList<QGraphicsItem*> grItems;

    QVector< QVector<int> > secs;
    double TotalEnergyDeposited;

    bool bForbidUpdate;

    int binsEnergy = 100;
    double fromEnergy = 0;
    double toEnergy = 0;
    int selectedModeForEnergyDepo = 0;
    int binsDistance = 100;
    double fromDistance = 0;
    double toDistance = 0;
    int binsB1 = 100;
    double fromB1 = 0;
    double toB1 = 0;
    int binsB2 = 100;
    double fromB2 = 0;
    double toB2 = 0;

    QVector<bool> ExpandedItems;

    void clearGrItems();
    void updateSignalLabels(float MaxSignal);
    void addPMitems  (const QVector<float>* vector, float MaxSignal, DynamicPassivesHandler *Passives);
    void addTextitems(const QVector<float>* vector, float MaxSignal, DynamicPassivesHandler *Passives);
    void updateSignalScale();    
    void updateSignalTableWidth();
    void addParticleHistoryLogLine(int iRec, int level);
    void updateMonitors();
    void updatePTHistoryBinControl();
    void fillEvTabViewRecord(QTreeWidgetItem *item, const AParticleTrackingRecord *pr, int ExpansionLevel) const;
    void EV_show();
    void EV_showTree();
    void EV_showGeo();
    int findEventWithFilters(int currentEv, bool bUp);
    void doProcessExpandedStatus(QTreeWidgetItem *item, int &counter, bool bStore);
};

#endif // OUTPUTWINDOW_H
