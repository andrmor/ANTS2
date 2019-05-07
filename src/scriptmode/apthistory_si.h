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

class APTHistory_SI : public AScriptInterface
{
    Q_OBJECT

public:
    APTHistory_SI(ASimulationManager & SimulationManager);
    ~APTHistory_SI();

    //virtual bool InitOnRun() override;
    //virtual void ForceStop() override;

public slots:
    int countEvents();
    int countPrimaries(int iEvent);
    QString recordToString(int iEvent, int iPrimary, bool includeSecondaries);

    void cd_set(int iEvent, int iPrimary);
    QVariantList cd_getTrackRecord();
    bool cd_step();
    void cd_firstStep();
    void cd_lastStep();
    QVariantList cd_getStepRecord();
    int  cd_countSecondaries();
    void cd_in(int indexOfSecondary);
    bool cd_out();

    void clearCriteria();
    void setParticle(QString particleName);
    void setOnlyPrimary();
    void setOnlySecondary();
    void setMaterial(int matIndex);
    void setVolume(QString volumeName);

    void setFromMaterial(int matIndex);
    void setToMaterial(int matIndex);
    void setFromVolume(QString volumeName);
    void setToVolume(QString volumeName);
    void setFromIndex(int volumeIndex);
    void setToIndex(int volumeIndex);

    QVariantList findParticles();
    QVariantList findProcesses();
    QVariantList findDepositedEnergies(int bins, double from, double to);
    QVariantList findTravelledDistances(int bins, double from, double to);
    QVariantList findOnBorder(QString what, QString cuts, int bins, double from, double to);
    QVariantList findOnBorder(QString what, QString vsWhat, QString cuts, int bins1, double from1, double to1, int bins2, double from2, double to2);

private:
    const ASimulationManager & SM;
    const std::vector<AEventTrackingRecord *> & TH;

    ATrackingHistoryCrawler * Crawler;
    AFindRecordSelector * Criteria = nullptr;

    const AParticleTrackingRecord * Rec = nullptr; //current record for cd
    int Step = 0;  //current step for cd

};


#endif // APTHISTORY_SI_H
