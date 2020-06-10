#include "aparticlesimsettings.h"
#include "ajsontools.h"
#include "aparticlesourcerecord.h"

#include <QDebug>

void AParticleSimSettings::clearSettings()
{
    GenerationMode = Sources;
    EventsToDo     = 1;
    bMultiple      = false;
    MeanPerEvent   = 1;
    MultiMode      = Constant;
    bDoS1          = true;
    bDoS2          = false;
    bIgnoreNoHits  = false;
    bIgnoreNoDepo  = false;
    bClusterMerge  = false;
    ClusterRadius  = 0.1;
    ClusterTime    = 1.0;

    SourceGenSettings.clear();
    FileGenSettings.clear();
    ScriptGenSettings.clear();
}

void AParticleSimSettings::writeToJson(QJsonObject & json) const
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

    SourceGenSettings.writeToJson(json);

    {
        QJsonObject js;
            FileGenSettings.writeToJson(js);
        json["GenerationFromFile"] = js;
    }

    {
        QJsonObject js;
            ScriptGenSettings.writeToJson(js);
        json["GenerationFromScript"] = js;
    }
}

void AParticleSimSettings::readFromJson(const QJsonObject & json)
{
    clearSettings();

    {
        QJsonObject js;
        bool ok = parseJson(json, "SourceControlOptions", js);
        if (!ok)
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
    }
    if (GenerationMode != Sources) bMultiple = false;  // TODO: move bMultiple to Sources!

    SourceGenSettings.readFromJson(json);

    {
        QJsonObject js;
            bool ok = parseJson(json, "GenerationFromFile", js);
        if (ok) FileGenSettings.readFromJson(js);
    }

    {
        QJsonObject js;
            bool ok = parseJson(json, "GenerationFromScript", js);
        if (ok) ScriptGenSettings.readFromJson(js);
    }
}

// ------------------------------------

void AScriptGenSettings::writeToJson(QJsonObject & json) const
{
    json["Script"] = Script;
}

void AScriptGenSettings::readFromJson(const QJsonObject & json)
{
    clear();

    parseJson(json, "Script", Script);
}

void AScriptGenSettings::clear()
{
    Script.clear();
}

void AFileGenSettings::writeToJson(QJsonObject &json) const
{
    json["FileName"]         = FileName;
    json["FileFormat"]       = FileFormat; //static_cast<int>(FileFormat);
    json["NumEventsInFile"]  = NumEventsInFile;
    json["LastValidation"]   = LastValidationMode; //static_cast<int>(LastValidationMode);
    json["FileLastModified"] = FileLastModified.toMSecsSinceEpoch();

    QJsonArray ar; ar.fromStringList(ValidatedWithParticles);
    json["ValidatedWithParticles"] = ar;
}

void AFileGenSettings::readFromJson(const QJsonObject & json)
{
    clear();

    parseJson(json, "FileName", FileName);

    FileFormat = Undefined;
    if (json.contains("FileFormat"))
    {
        int im;
        parseJson(json, "FileFormat", im);
        if (im >= 0 && im < 5)
            FileFormat = static_cast<FileFormatEnum>(im);
    }

    parseJson(json, "NumEventsInFile", NumEventsInFile);

    int iVal = static_cast<int>(None);
    parseJson(json, "LastValidation", iVal);
    LastValidationMode = static_cast<ValidStateEnum>(iVal);
    ValidationMode = LastValidationMode;

    qint64 lastMod;
    parseJson(json, "FileLastModified", lastMod);
    FileLastModified = QDateTime::fromMSecsSinceEpoch(lastMod);

    QJsonArray ar;
    parseJson(json, "ValidatedWithParticles", ar);
    ValidatedWithParticles.clear();
    for (int i=0; i<ar.size(); i++) ValidatedWithParticles << ar.at(i).toString();
}

void AFileGenSettings::clear()
{
    FileName.clear();
    FileFormat         = Undefined;
    NumEventsInFile    = 0;
    ValidationMode     = None;
    LastValidationMode = None;
    FileLastModified   = QDateTime();
    ValidatedWithParticles.clear();
}

#include "aglobalsettings.h"
void ASourceGenSettings::writeToJson(QJsonObject &json) const
{
    AMaterialParticleCollection * MpCollection = AGlobalSettings::getInstance().getMpCollection();

    QJsonArray ja;
    for (const AParticleSourceRecord * ps : ParticleSourcesData)
    {
        QJsonObject js;
        ps->writeToJson(js, *MpCollection);
        ja.append(js);
    }
    json["ParticleSources"] = ja;
}

#include "amaterialparticlecolection.h"
void ASourceGenSettings::readFromJson(const QJsonObject &  json)
{
    clear();

    if (!json.contains("ParticleSources"))  // !*! move to outside
    {
        qWarning() << "--- Json does not contain config for particle sources!";
        return;
    }

    QJsonArray ar = json["ParticleSources"].toArray();
    if (ar.isEmpty()) return;

    AMaterialParticleCollection * MpCollection = AGlobalSettings::getInstance().getMpCollection();

    for (int iSource = 0; iSource < ar.size(); iSource++)
    {
        QJsonObject js = ar.at(iSource).toObject();
        AParticleSourceRecord * ps = new AParticleSourceRecord();
        bool ok = ps->readFromJson(js, *MpCollection);
        if (ok) ParticleSourcesData << ps;
        else
        {
            qWarning() << "||| Load particle source #" << iSource << "from json failed!";
            delete ps;
        }
    }

    calculateTotalActivity();
}

void ASourceGenSettings::clear()
{
    for (AParticleSourceRecord * r : ParticleSourcesData) delete r;
    ParticleSourcesData.clear();

    TotalActivity = 0;
}

int ASourceGenSettings::getNumSources() const
{
    return ParticleSourcesData.size();
}

const AParticleSourceRecord * ASourceGenSettings::getSourceRecord(int iSource) const
{
    if (iSource < 0 || iSource >= ParticleSourcesData.size()) return nullptr;
    return ParticleSourcesData.at(iSource);
}

AParticleSourceRecord *ASourceGenSettings::getSourceRecord(int iSource)
{
    if (iSource < 0 || iSource >= ParticleSourcesData.size()) return nullptr;
    return ParticleSourcesData.at(iSource);
}

void ASourceGenSettings::calculateTotalActivity()
{
    TotalActivity = 0;
    for (const AParticleSourceRecord * r : ParticleSourcesData)
        TotalActivity += r->Activity;
}

void ASourceGenSettings::append(AParticleSourceRecord * gunParticle)
{
    ParticleSourcesData.append(gunParticle);
    calculateTotalActivity();
}

void ASourceGenSettings::forget(AParticleSourceRecord *gunParticle)
{
    ParticleSourcesData.removeAll(gunParticle);
    calculateTotalActivity();
}

bool ASourceGenSettings::replace(int iSource, AParticleSourceRecord * gunParticle)
{
    if (iSource < 0 || iSource >= ParticleSourcesData.size()) return false;

    delete ParticleSourcesData[iSource];
    ParticleSourcesData[iSource] = gunParticle;
    calculateTotalActivity();
    return true;
}

void ASourceGenSettings::remove(int iSource)
{
    if (ParticleSourcesData.isEmpty()) return;
    if (iSource < 0 || iSource >= ParticleSourcesData.size()) return;

    delete ParticleSourcesData[iSource];
    ParticleSourcesData.remove(iSource);
    calculateTotalActivity();
}
