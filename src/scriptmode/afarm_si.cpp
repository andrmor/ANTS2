#include "afarm_si.h"
#include "agridrunner.h"
#include "aremoteserverrecord.h"

#include <QJsonObject>

AFarm_si::AFarm_si(const QJsonObject & Config, AGridRunner & GridRunner) :
    AScriptInterface(), Config(Config), GridRunner(GridRunner){}

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

QVariantList AFarm_si::evaluateScript(QString Script, QVariantList Resources, QVariantList FileNames)
{
    QVariant res = GridRunner.EvaluateSript(Script, Config, Resources, FileNames);

    if (res.type() == QVariant::String)
    {
        abort(res.toString());
        return QVariantList();
    }
    else
        return res.toList();
}
