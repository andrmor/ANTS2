#ifndef AFILEPARTICLEGENERATOR_H
#define AFILEPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>
#include <QFile>
#include <QRegularExpression>
#include <QVector>
#include <QJsonObject>

class QTextStream;

class AParticleFileStat
{
public:
    int numEvents = 0;
    int numMultipleEvents = 0;
    QVector<int> ParticleStat;
    QString ErrorString;
};

class AFileParticleGenerator : public AParticleGun
{
public:
    virtual ~AFileParticleGenerator(){}

    void          SetFileName(const QString &fileName) {FileName = fileName;}
    const QString GetFileName() const {return FileName;}

    virtual bool Init() override;               //called before first use
    virtual void ReleaseResources() override;   //called after end of operation
    virtual QVector<AGeneratedParticle>* GenerateEvent();

    virtual bool CheckConfiguration() override; //check consistency of the configuration

    virtual void RemoveParticle(int particleId) override {} //cannot be used for this class
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);

private:
    QString FileName;
    QFile File;
    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'

    QTextStream* Stream = 0;

private:
    //bool readLine();

public:
    static const AParticleFileStat InspectFile(const QString& fname, int ParticleCount); //TODO remove static
};

#endif // AFILEPARTICLEGENERATOR_H
