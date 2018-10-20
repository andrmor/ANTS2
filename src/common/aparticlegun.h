#ifndef APARTICLEGUN_H
#define APARTICLEGUN_H

#include <QVector>
#include <QString>

class QJsonObject;

class AGeneratedParticle
{
public:
    AGeneratedParticle(int pId, double energy, double x, double y, double z, double vx, double vy, double vz) :
        ParticleId(pId), Energy(energy)
    {
        Position[0] = x;
        Position[1] = y;
        Position[2] = z;
        Direction[0] = vx;
        Direction[1] = vy;
        Direction[2] = vz;
    }
    AGeneratedParticle(){}

    int    ParticleId;
    double Energy;
    double Position[3];
    double Direction[3];
};

class AParticleGun
{
public:
    virtual ~AParticleGun(){}

    virtual bool Init() = 0;             //called before first use
    virtual void ReleaseResources() {}   //called after end of operation
    virtual QVector<AGeneratedParticle>* GenerateEvent() = 0;

    virtual const QString CheckConfiguration() const = 0; //check consistency of the configuration

    virtual void RemoveParticle(int particleId) = 0; //should NOT be used to remove one of particles in use! use onIsPareticleInUse first
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const = 0;

    virtual void writeToJson(QJsonObject &json) const = 0;
    virtual bool readFromJson(const QJsonObject &json) = 0;

    virtual void SetStartEvent(int) {} // for 'from file' generator

    const QString& GetErrorString() const {return ErrorString;}

protected:
    QString ErrorString;
};

#endif // APARTICLEGUN_H
