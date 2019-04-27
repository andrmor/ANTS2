#ifndef APTHISTORY_SI_H
#define APTHISTORY_SI_H

#include "ascriptinterface.h"

#include <vector>

#include <QObject>
#include <QString>
#include <QVariant>

class ASimulationManager;
class AEventTrackingRecord;

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
    QString printRecord(int iEvent, int iPrimary, bool includeSecondaries);

private:
    const ASimulationManager & SM;
    const std::vector<AEventTrackingRecord *> & TH;

};


#endif // APTHISTORY_SI_H
