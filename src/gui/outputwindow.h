#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>

class MainWindow;
class myQGraphicsView;
class QStandardItemModel;
struct AReconRecord;
class EventsDataClass;
class DynamicPassivesHandler;

namespace Ui {
class OutputWindow;
}

class OutputWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit OutputWindow(QWidget* parent, MainWindow *mw, EventsDataClass *eventsDataHub);
    ~OutputWindow();

    QVector< QBitArray > SiPMpixels; //[PM#] [time] [pixY] [pixX] - to report only!

    void PMnumChanged();
    void SetCurrentEvent(int iev);

    void OutTextClear();
    void OutText(QString str);

    void SetTab(int index);

    void RefreshData();
    void ResetViewport(); //auto-size and center viewing area
    void InitWindow();

    void ShowEventHistoryLog();
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
    void on_pbClearText_clicked();
    void on_pbAverageSignals_clicked();
    void on_pbShowParticldeLog_clicked();
    void on_pbShowPhotonLog_clicked();
    void on_pbShowSumSignals_clicked();
    void on_pbShowSignals_clicked();
    void on_pbShowPhotonProcessesLog_clicked();
    void on_pbShowPhotonLossLog_clicked();    
    void on_pbShowSelected_clicked();
    void on_cobWhatToShow_currentIndexChanged(int index);
    void on_pbNextEvent_clicked();

    void on_tabwinDiagnose_tabBarClicked(int index);
    void RefreshPMhitsTable();

protected:
    bool event(QEvent *event);   

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

    void clearGrItems();
    void updateSignalLabels(double MaxSignal);
    void addPMitems(bool fHaveData, int CurrentEvent, double MaxSignal, DynamicPassivesHandler *Passives);
    void addTextitems(bool fHaveData, int CurrentEvent, double MaxSignal, DynamicPassivesHandler *Passives);
    void updateSignalScale();    
    void updateSignalTableWidth();
    void showParticleHistString(int iRec, int level);
    void addParticleHistoryLogLine(int iRec, int level);
};

#endif // OUTPUTWINDOW_H
