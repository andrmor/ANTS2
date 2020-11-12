#ifndef APTHISTORY_SI_H
#define APTHISTORY_SI_H

#include "ascriptinterface.h"

#include <vector>

#include <QObject>
#include <QString>
#include <QVariant>

class ASimulationManager;
class AEventTrackingRecord;
class AParticleTrackingRecord;
class ATrackingHistoryCrawler;
class AFindRecordSelector;
class AHistorySearchProcessor_findDepositedEnergy;
class AHistorySearchProcessor_Border;

class APTHistory_SI : public AScriptInterface
{
    Q_OBJECT

public:
    APTHistory_SI(ASimulationManager & SimulationManager);
    ~APTHistory_SI();

    //bool InitOnRun() override;
    //void ForceStop() override;

public slots:
    int countEvents();
    int countPrimaries(int iEvent);
    QString recordToString(int iEvent, int iPrimary, bool includeSecondaries);

    void clearHistory();

    void cd_set(int iEvent, int iPrimary);
    QVariantList cd_getTrackRecord();
    QString cd_getProductionProcess();  // returns empty string if it is primary particle
    bool cd_step();
    bool cd_step(int iStep);
    bool cd_stepToProcess(QString processName);
    int  cd_getCurrentStep();
    void cd_firstStep();
    bool cd_firstNonTransportationStep();
    void cd_lastStep();
    QVariantList cd_getStepRecord();
    QVariantList cd_getDirections();
    int  cd_countSteps();
    int  cd_countSecondaries();
    bool cd_hadPriorInteraction();
    void cd_in(int indexOfSecondary);
    bool cd_out();                      // sets Step to 0   TODO: add QVector with Step before "in" and restore on "out"

    void clearCriteria();
    void setParticle(QString particleName);
    void setOnlyPrimary();
    void setOnlySecondary();
    void setLimitToFirstInteractionOfPrimary();
    void setMaterial(int matIndex);
    void setVolume(QString volumeName);
    void setVolumeIndex(int volumeIndex);

    void setFromMaterial(int matIndex);
    void setToMaterial(int matIndex);
    void setFromVolume(QString volumeName);
    void setToVolume(QString volumeName);
    void setFromIndex(int volumeIndex);
    void setToIndex(int volumeIndex);
    void setOnlyCreated();
    void setOnlyEscaping();

    QVariantList findParticles();
    QVariantList findProcesses(int All0_WithDepo1_TrackEnd2 = 0);
    QVariantList findDepositedEnergies(int bins, double from, double to);
    QVariantList findDepositedEnergiesWithSecondaries(int bins, double from, double to);
    QVariantList findDepositedEnergiesOverEvent(int bins, double from, double to);
    QVariantList findDepositedEnergyStats();
    QVariantList findDepositedEnergyStats(double timeFrom, double timeTo);
    QVariantList findTravelledDistances(int bins, double from, double to);

    QVariantList findOnBorder(QString what, QString cuts, int bins, double from, double to);
    QVariantList findOnBorder(QString what, QString vsWhat, QString cuts, int bins, double from, double to);
    QVariantList findOnBorder(QString what, QString vsWhat, QString cuts, int bins1, double from1, double to1, int bins2, double from2, double to2);
    QVariantList findOnBorder(QString what, QString vsWhat, QString andVsWhat, QString cuts, int bins1, double from1, double to1, int bins2, double from2, double to2);

private:
    const ASimulationManager & SM;
    std::vector<AEventTrackingRecord *> & TH;

    ATrackingHistoryCrawler * Crawler = nullptr;
    AFindRecordSelector * Criteria = nullptr;

    const AParticleTrackingRecord * Rec = nullptr; // current record for cd
    int Step = 0;                                  // current step for cd

    QVariantList findDepE(AHistorySearchProcessor_findDepositedEnergy & p);
    const QVariantList findOB_1D(AHistorySearchProcessor_Border & p);
    const QVariantList findOB_2D(AHistorySearchProcessor_Border & p);
};


#endif // APTHISTORY_SI_H
