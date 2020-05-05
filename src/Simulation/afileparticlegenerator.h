#ifndef AFILEPARTICLEGENERATOR_H
#define AFILEPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>
#include <QStringList>
#include <QFile>
#include <QRegularExpression>
#include <QVector>
#include <QJsonObject>
#include <QDateTime>

#include <string>
#include <vector>

class AMaterialParticleCollection;
class QTextStream;
class AFilePGEngine;

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

class AFileParticleGenerator : public AParticleGun
{
public:
    AFileParticleGenerator(const AMaterialParticleCollection & MpCollection);
    virtual         ~AFileParticleGenerator();

    enum            ValidStateEnum {None = 0, Relaxed, Strict};
    enum            FileFormatEnum {Undefined = 0, BadFormat = 1, Simplistic = 2, G4Ascii = 3, G4Binary = 4};

    bool            Init() override;               //has to be called before first use of GenerateEvent()
    void            ReleaseResources() override;
    bool            GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) override;

    void            RemoveParticle(int) override {} //cannot be used for this class
    bool            IsParticleInUse(int , QString &) const override { return false; } // not relevant for this class, validator is on guard

    void            writeToJson(QJsonObject& json) const override;
    bool            readFromJson(const QJsonObject& json) override;

    void            SetStartEvent(int startEvent) override;

    void            SetFileName(const QString &fileName);
    QString         GetFileName() const {return FileName;}

    FileFormatEnum  GetFileFormat() const {return FileFormat;}
    bool            IsFormatG4() const;
    bool            IsFormatBinary() const;
    QString         GetFormatName() const;

    void            SetValidationMode(ValidStateEnum Mode);
    void            InvalidateFile();    //forces the file to be inspected again during next call of Init()
    bool            IsValidated() const;
    bool            IsValidParticle(int ParticleId) const;                // result depends on current ValidationType
    bool            IsValidParticle(const QString & ParticleName) const;  // result depends on current ValidationType

    bool            generateG4File(int eventBegin, int eventEnd, const QString & FileName);

    void            setParticleMustBeDefined(bool flag);

public:
    int          NumEventsInFile          = 0;      // is saved in config

    int          statNumEmptyEventsInFile = 0;
    int          statNumMultipleEvents    = 0;

    bool         bCollectExpandedStatistics = false;
    std::vector<AParticleInFileStatRecord> ParticleStat;

    const AMaterialParticleCollection & MpCollection;

private:
    AFilePGEngine * Engine      = nullptr;

    QString         FileName;
    FileFormatEnum  FileFormat      = Undefined;
    ValidStateEnum  ValidationMode  = None;

    QDateTime       FileLastModified;          // saved - used in validity check
    QStringList     ValidatedWithParticles;    // saved - used in validaty check
    ValidStateEnum  LastValidationMode = None; // saved - used in validaty check

private:
    void clearFileData();
    bool DetermineFileFormat();
    bool isFileG4Binary();
    bool isFileG4Ascii();
    bool isFileSimpleAscii();
};

class AFilePGEngine
{
public:
    AFilePGEngine(AFileParticleGenerator * fpg) : FPG(fpg), FileName(fpg->GetFileName()) {}
    virtual ~AFilePGEngine(){}

    virtual bool doInit(bool bNeedInspect, bool bDetailedInspection) = 0;
    virtual bool doGenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) = 0;
    virtual bool doSetStartEvent(int startEvent) = 0;
    virtual bool doGenerateG4File(int eventBegin, int eventEnd, const QString & FileName) = 0;

protected:
    AFileParticleGenerator * FPG = nullptr;
    QString FileName;

    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\:|\\t)");  // separators are: ' ' or ',' or ':' or '\t'
};

class AFilePGEngineSimplistic : public AFilePGEngine
{
public:
    AFilePGEngineSimplistic(AFileParticleGenerator * fpg) : AFilePGEngine(fpg) {}
    ~AFilePGEngineSimplistic();

    bool doInit(bool bNeedInspect, bool bDetailedInspection) override;
    bool doGenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;
    bool doSetStartEvent(int startEvent) override;
    bool doGenerateG4File(int eventBegin, int eventEnd, const QString & FileName) override; // not in use! SimManager uses mainstream approach to generate events

private:
    QFile File;
    QTextStream * Stream = nullptr;
};

class AFilePGEngineG4antsTxt : public AFilePGEngine
{
public:
    AFilePGEngineG4antsTxt(AFileParticleGenerator * fpg) : AFilePGEngine(fpg) {}
    ~AFilePGEngineG4antsTxt();

    bool doInit(bool bNeedInspect, bool bDetailedInspection) override;
    bool doGenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;
    bool doSetStartEvent(int startEvent) override;
    bool doGenerateG4File(int eventBegin, int eventEnd, const QString & FileName) override;

private:
    std::ifstream * inStream = nullptr;
};

class AFilePGEngineG4antsBin : public AFilePGEngine
{
public:
    AFilePGEngineG4antsBin(AFileParticleGenerator * fpg) : AFilePGEngine(fpg) {}
    ~AFilePGEngineG4antsBin();

    bool doInit(bool bNeedInspect, bool bDetailedInspection) override;
    bool doGenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;
    bool doSetStartEvent(int startEvent) override;
    bool doGenerateG4File(int eventBegin, int eventEnd, const QString & FileName) override;

private:
    std::ifstream * inStream = nullptr;
};

#endif // AFILEPARTICLEGENERATOR_H
