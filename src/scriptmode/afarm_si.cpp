#include "afarm_si.h"
#include "agridrunner.h"
#include "aremoteserverrecord.h"

#include <QJsonObject>
#include <QFile>

AFarm_si::AFarm_si(const QJsonObject & Config, AGridRunner & GridRunner) :
    AScriptInterface(), Config(Config), GridRunner(GridRunner)
{
    H["getServers"] = "Returns the list of all configured servers\nFormat: [ [NumThreads1, SpeedFactor1], [NumThreads2, SpeedFactor2], ... ])";
    H["evaluateScript"] = "Evaluate the script on the ants2 farm. The user has to provide one or both (in this case mathing length) of:\n"
                          "1) an array of per-worker resources\n"
                          "2) an array of per-worker file names (the files will be automatically sent to the remote servers)\n"
                          "Each remote worker will be attributed with one element of the resources and a file (matching indexes in the arrays)\n"
                          "On the remote server, the resource is accessed as 'Data' variable\n"
                          "On the remote server, the uploaded file is saved with the name 'File.dat'";
}

void AFarm_si::ForceStop()
{
    if (&GridRunner == nullptr) qDebug() << "!-- Grid runner is absent!";
    else GridRunner.Abort();
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

void AFarm_si::reconstruct()
{
    GridRunner.Reconstruct(&Config);
}

void AFarm_si::simulate()
{
    GridRunner.Simulate(&Config);
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

void AFarm_si::uploadFile(int ServerIndex, QString FileName)
{
    QString err = GridRunner.UploadFile(ServerIndex, FileName);
    if (!err.isEmpty()) abort(err);
}

