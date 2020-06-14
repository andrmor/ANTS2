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

class AFileParticleGenerator : public AParticleGun
{
public:
    AFileParticleGenerator(const AFileGenSettings &Settings, const AMaterialParticleCollection & MpCollection);
    virtual         ~AFileParticleGenerator();

    bool            Init() override;  // cannot modify settings, so validation is not possible with this method
    bool            InitWithCheck(AFileGenSettings & settings, bool bExpanded);  // need to be called if validation is needed!

    void            ReleaseResources() override;
    bool            GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) override;

    void            SetStartEvent(int startEvent) override;

    bool            generateG4File(int eventBegin, int eventEnd, const QString & FileName);

public:
    const AFileGenSettings & ConstSettings;
    const AMaterialParticleCollection & MpCollection;

private:
    AFilePGEngine * Engine = nullptr;

    bool determineFileFormat(AFileGenSettings & settings);
    bool isFileG4Binary(AFileGenSettings & settings);
    bool isFileG4Ascii(AFileGenSettings & settings);
    bool isFileSimpleAscii(AFileGenSettings & settings);
};

class AFilePGEngine
{
public:
    AFilePGEngine(AFileParticleGenerator * fpg) : FPG(fpg) {}
    virtual ~AFilePGEngine(){}

    virtual bool doInit() = 0;
    virtual bool doInitAndInspect(AFileGenSettings & settings, bool bDetailedInspection) = 0;
    virtual bool doGenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) = 0;
    virtual bool doSetStartEvent(int startEvent) = 0;
    virtual bool doGenerateG4File(int eventBegin, int eventEnd, const QString & FileName) = 0;

protected:
    AFileParticleGenerator * FPG = nullptr;

    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\:|\\t)");  // separators are: ' ' or ',' or ':' or '\t'
};

class AFilePGEngineSimplistic : public AFilePGEngine
{
public:
    AFilePGEngineSimplistic(AFileParticleGenerator * fpg) : AFilePGEngine(fpg) {}
    ~AFilePGEngineSimplistic();

    bool doInit() override;
    bool doInitAndInspect(AFileGenSettings & settings, bool bDetailedInspection) override;
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

    bool doInit() override;
    bool doInitAndInspect(AFileGenSettings & settings, bool bDetailedInspection) override;
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

    bool doInit() override;
    bool doInitAndInspect(AFileGenSettings & settings, bool bDetailedInspection) override;
    bool doGenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;
    bool doSetStartEvent(int startEvent) override;
    bool doGenerateG4File(int eventBegin, int eventEnd, const QString & FileName) override;

private:
    std::ifstream * inStream = nullptr;
};

#endif // AFILEPARTICLEGENERATOR_H
