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

class APTHistory_SI : public AScriptInterface
{
    Q_OBJECT

public:
    APTHistory_SI(ASimulationManager & SimulationManager);
    ~APTHistory_SI(){}

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

private:
    const ASimulationManager & SM;
    const std::vector<AEventTrackingRecord *> & TH;

    const AParticleTrackingRecord * Rec = nullptr; //current record for cd
    int Step = 0;  //current step for cd

};


#endif // APTHISTORY_SI_H
