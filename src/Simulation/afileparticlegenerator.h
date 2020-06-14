#ifndef AFILEPARTICLEGENERATOR_H
#define AFILEPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>
#include <QFile>
#include <QRegularExpression>
#include <QVector>

#include <string>
#include <vector>

class AFileGenSettings;
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
    AFileParticleGenerator(AFileGenSettings & Settings, const AMaterialParticleCollection & MpCollection);
    virtual         ~AFileParticleGenerator();

    bool            Init() override;               //has to be called before first use of GenerateEvent()
    void            ReleaseResources() override;
    bool            GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) override;

    void            SetStartEvent(int startEvent) override;

    void            SetFileName(const QString &fileName);
    QString         GetFileName() const;

    bool            IsFormatG4() const;      // !*! to remove
    bool            IsFormatBinary() const;  // !*! to remove

    void            InvalidateFile();    //forces the file to be inspected again during next call of Init()
    bool            IsValidated() const;
    bool            IsValidParticle(int ParticleId) const;                // result depends on current ValidationType
    bool            IsValidParticle(const QString & ParticleName) const;  // result depends on current ValidationType

    bool            generateG4File(int eventBegin, int eventEnd, const QString & FileName);

    void            setParticleMustBeDefined(bool flag);

public:
    int          statNumEmptyEventsInFile = 0;
    int          statNumMultipleEvents    = 0;

    bool         bCollectExpandedStatistics = false;
    std::vector<AParticleInFileStatRecord> ParticleStat;

    AFileGenSettings & Settings;
    const AMaterialParticleCollection & MpCollection;

private:
    AFilePGEngine * Engine = nullptr;

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
