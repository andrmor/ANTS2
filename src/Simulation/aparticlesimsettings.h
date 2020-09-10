#ifndef APARTICLEMODESETTINGS_H
#define APARTICLEMODESETTINGS_H

#include <QString>
#include <QVector>
#include <QDateTime>

#include <vector>

class QJsonObject;
class AParticleSourceRecord;
class ASourceParticleGenerator;

class ASourceGenSettings   // ! this class uses MpCollection obtained through singleton GlobalSettings. read from json for non-existing particles triggers global update!
{
public:
    QVector<AParticleSourceRecord*> ParticleSourcesData;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    void clear();

    int    getNumSources() const;

    const AParticleSourceRecord * getSourceRecord(int iSource) const;
    AParticleSourceRecord * getSourceRecord(int iSource);

    void   calculateTotalActivity();
    double getTotalActivity() const {return TotalActivity;}

    void   append(AParticleSourceRecord * gunParticle);
    void   forget(AParticleSourceRecord * gunParticle);
    bool   replace(int iSource, AParticleSourceRecord * gunParticle);
    void   remove(int iSource);

    QString isParticleInUse(int particleId) const;
    bool removeParticle(int particleId);

private:
    double TotalActivity = 0;
};

struct AParticleInFileStatRecord
{
    AParticleInFileStatRecord(const std::string & NameStd, double Energy);
    AParticleInFileStatRecord(const QString     & NameQt,  double Energy);
    AParticleInFileStatRecord() {}

    std::string NameStd;
    QString     NameQt;
    int         Entries = 0;
    double      Energy  = 0;
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

    bool            isValidated() const;
    void            invalidateFile();    //forces the file to be inspected again during next call of Init()

    QString         getFormatName() const;
    bool            isFormatG4() const;
    bool            isFormatBinary() const;

    void            writeToJson(QJsonObject & json) const;
    void            readFromJson(const QJsonObject & json);

    void            clear();
    void            clearStatistics();

    bool            isValidParticle(int ParticleId) const;                // result depends on current ValidationType
    bool            isValidParticle(const QString & ParticleName) const;  // result depends on current ValidationType

    //runtime
    int             statNumEmptyEventsInFile = 0;
    int             statNumMultipleEvents    = 0;
    std::vector<AParticleInFileStatRecord> ParticleStat;

private:
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

    QString isParticleInUse(int particleId) const;  // returns "" if not in use, otherwise gives detail where it is used
    bool removeParticle(int particleId); //should NOT be used to remove one of particles in use! use isPareticleInUse first
};

#endif // APARTICLEMODESETTINGS_H
