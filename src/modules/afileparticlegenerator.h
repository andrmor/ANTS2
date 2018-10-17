#ifndef AFILEPARTICLEGENERATOR_H
#define AFILEPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>
#include <QFile>

class QTextStream;

class AFileParticleGenerator : public AParticleGun
{
public:
    AFileParticleGenerator(const QString& FileName);
    virtual ~AFileParticleGenerator(){}

    virtual void Init() override;               //called before first use
    virtual void ReleaseResources() override;   //called after end of operation
    virtual QVector<AGeneratedParticle>* GenerateEvent();

    virtual bool CheckConfiguration() override; //check consistency of the configuration

    virtual void RemoveParticle(int particleId) override {} //cannot be used for this class
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

private:
    const QString FileName;
    QFile File;

    QTextStream* Stream = 0;

private:
    //bool readLine();
};

#endif // AFILEPARTICLEGENERATOR_H
