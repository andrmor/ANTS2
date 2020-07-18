#include "afarm_si.h"
#include "agridrunner.h"
#include "aremoteserverrecord.h"

#include <QJsonObject>

AFarm_si::AFarm_si(const QJsonObject & Config, AGridRunner & GridRunner) :
    AScriptInterface(), Config(Config), GridRunner(GridRunner)
{
    H["getServers"] = "Returns the list of configured (and not disabled) servers\nFormat: [ [NumThreads1, SpeedFactor1], [NumThreads2, SpeedFactor2], ... ])";
}

void AFarm_si::setTimeout(double Timeout_ms)
{
    GridRunner.SetTimeout(Timeout_ms);
}

QVariantList AFarm_si::getServers()
{
    QVariantList res;

    QString err = GridRunner.CheckStatus();
    if (!err.isEmpty())
    {
        abort(err);
        return res;
    }

    for (const ARemoteServerRecord * r : GridRunner.ServerRecords)
    {
        QVariantList el;
        el << r->NumThreads_Allocated << r->SpeedFactor;
        res.push_back(el);
    }
    return res;
}

QVariantList AFarm_si::evaluateScript(QString Script, QVariantList Resources, QVariantList FileNames)
{
    QVariant res = GridRunner.EvaluateSript(Script, Config, Resources, FileNames);

    if (res.type() == QVariant::String)
    {
        //error is coded as a string
        abort(res.toString());
        return QVariantList();
    }
    else
        return res.toList();
}

