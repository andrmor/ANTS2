#ifndef APARTICLEMODESETTINGS_H
#define APARTICLEMODESETTINGS_H

#include <QString>
#include <QVector>
#include <QDateTime>

class QJsonObject;
class AParticleSourceRecord;
class ASourceParticleGenerator;

class ASourceGenSettings
{
public:
    QVector<AParticleSourceRecord*> ParticleSourcesData;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    void clear();

    int    getNumSources() const;
    void   calculateTotalActivity();
    double getTotalActivity() const {return TotalActivity;}

    void   append(AParticleSourceRecord * gunParticle);
    void   forget(AParticleSourceRecord* gunParticle);
    bool   replace(int iSource, AParticleSourceRecord* gunParticle);
    void   remove(int iSource);

private:
    double TotalActivity = 0;
};

class AFileGenSettings
{
public:
    enum            ValidStateEnum {None = 0, Relaxed = 1, Strict = 2};
    enum            FileFormatEnum {Undefined = 0, BadFormat = 1, Simplistic = 2, G4Ascii = 3, G4Binary = 4};

    QString         FileName;
    FileFormatEnum  FileFormat         = Undefined;
    int             NumEventsInFile    = 0;
    ValidStateEnum  ValidationMode     = None;
    ValidStateEnum  LastValidationMode = None;
    QDateTime       FileLastModified;
    QStringList     ValidatedWithParticles;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    void clear();
};

class AScriptGenSettings
{
public:
    QString Script;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    void clear();
};

// -------------- Main -------------

class AParticleSimSettings
{
public:
    enum AGenMode   {Sources = 0, File = 1, Script = 2};
    enum AMultiMode {Constant = 0, Poisson = 1};

    AGenMode GenerationMode = Sources;
    int     EventsToDo      = 1;
    bool    bMultiple       = false;        //affects only "Sources", maybe move there?
    double  MeanPerEvent    = 1.0;          //affects only "Sources", maybe move there?
    AMultiMode MultiMode    = Constant;
    bool    bDoS1           = true;
    bool    bDoS2           = false;
    bool    bIgnoreNoHits   = false;
    bool    bIgnoreNoDepo   = false;
    bool    bClusterMerge   = false;
    double  ClusterRadius   = 0.1;
    double  ClusterTime     = 1.0;

    ASourceGenSettings SourceGenSettings;
    AFileGenSettings   FileGenSettings;
    AScriptGenSettings ScriptGenSettings;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    void clearSettings();
};

#endif // APARTICLEMODESETTINGS_H
