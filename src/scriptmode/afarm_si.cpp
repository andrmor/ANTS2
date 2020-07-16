#include "afarm_si.h"
#include "agridrunner.h"
#include "aremoteserverrecord.h"

AFarm_si::AFarm_si(AGridRunner &GridRunner) :
    AScriptInterface(),
    GridRunner(GridRunner)
{
}

AFarm_si::~AFarm_si()
{

}

void AFarm_si::setTimeout(double Timeout_ms)
{
    GridRunner.SetTimeout(Timeout_ms);
}

int AFarm_si::checkAvailableThreads()
{
    int numThr = 0;
    QString err = GridRunner.CheckStatus();
    if (!err.isEmpty()) abort(err);

    for (const ARemoteServerRecord * r : GridRunner.ServerRecords)
        if (r->NumThreads_Allocated > 0)
            numThr += r->NumThreads_Allocated;

    return numThr;
}
