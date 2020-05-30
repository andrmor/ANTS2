#include "aparticlesimsettings.h"
#include "ajsontools.h"

#include "asourceparticlegenerator.h"
#include "afileparticlegenerator.h"
#include "ascriptparticlegenerator.h"

#include <QDebug>

AParticleSimSettings::AParticleSimSettings(ASourceParticleGenerator *SoG, AFileParticleGenerator *FiG, AScriptParticleGenerator *ScG) :
    SourceGen(SoG), FileGen(FiG), ScriptGen(ScG) {}

void AParticleSimSettings::clearSettings()
{
    GenerationMode = Sources;
    EventsToDo      = 1;
    bMultiple       = false;
    MeanPerEvent    = 1;
    MultiMode    = Constant;
    bDoS1           = true;
    bDoS2           = false;
    bIgnoreNoHits   = false;
    bIgnoreNoDepo   = false;
    bClusterMerge   = false;
    ClusterRadius   = 0.1;
    ClusterTime     = 1.0;

    SourceGen->clear();
}

void AParticleSimSettings::writeToJson(QJsonObject &json) const
{
    {
        QJsonObject js;
            QString s;
                switch (GenerationMode)
                {
                case Sources: s = "Sources"; break;
                case File:    s = "File";    break;
                case Script:  s = "Script";  break;
                }
            js["ParticleGenerationMode"] = s;
            js["EventsToDo"] = EventsToDo;
            js["AllowMultipleParticles"] = bMultiple;
            js["AverageParticlesPerEvent"] = MeanPerEvent;
            js["TypeParticlesPerEvent"] = MultiMode;
            js["DoS1"] = bDoS1;
            js["DoS2"] = bDoS2;
            js["IgnoreNoHitsEvents"] = bIgnoreNoHits;
            js["IgnoreNoDepoEvents"] = bIgnoreNoDepo;
            js["ClusterMerge"] = bClusterMerge;
            js["ClusterMergeRadius"] = ClusterRadius;
            js["ClusterMergeTime"] = ClusterTime;
        json["SourceControlOptions"] = js;
    }

    SourceGen->writeToJson(json);

    {
        QJsonObject js;
            FileGen->writeToJson(js);
        json["GenerationFromFile"] = js;
    }

    {
        QJsonObject js;
            ScriptGen->writeToJson(js);
        json["GenerationFromScript"] = js;
    }
}

void AParticleSimSettings::readFromJson(const QJsonObject & json)
{
    clearSettings();

    QJsonObject js;
    bool bOK = parseJson(json, "SourceControlOptions", js);
    if (!bOK)
    {
        qWarning() << "Bad format of particle sim settings json";
        return;
    }

    QString PartGenMode = "Sources";
    parseJson(js, "ParticleGenerationMode", PartGenMode);
    if      (PartGenMode == "Sources") GenerationMode = Sources;
    else if (PartGenMode == "File")    GenerationMode = File;
    else if (PartGenMode == "Script")  GenerationMode = Script;
    else qWarning() << "Unknown particle generation mode";

    parseJson(js, "EventsToDo", EventsToDo);
    parseJson(js, "AllowMultipleParticles", bMultiple);
    parseJson(js, "AverageParticlesPerEvent", MeanPerEvent);
    int iMulti = 0;
    parseJson(js, "TypeParticlesPerEvent", iMulti);
    MultiMode = (iMulti == 1 ? Poisson : Constant);
    parseJson(js, "DoS1", bDoS1);
    parseJson(js, "DoS2", bDoS2);
    parseJson(js, "IgnoreNoHitsEvents", bIgnoreNoHits);
    parseJson(js, "IgnoreNoDepoEvents", bIgnoreNoDepo);
    parseJson(js, "ClusterMerge", bClusterMerge);
    parseJson(js, "ClusterMergeRadius", ClusterRadius);
    parseJson(js, "ClusterMergeTime", ClusterTime);

    SourceGen->readFromJson(js);

    QJsonObject fjs;
    bOK = parseJson(json, "GenerationFromFile", fjs);
    if (bOK) FileGen->readFromJson(fjs);

    QJsonObject sjs;
    bOK = parseJson(json, "GenerationFromScript", sjs);
    if (bOK) ScriptGen->readFromJson(sjs);
}
