#ifndef APARTICLEGUN_H
#define APARTICLEGUN_H

#include <QObject>
#include <QVector>
#include <QString>

class QJsonObject;
class AParticleRecord;

class AParticleGun : public QObject
{
    Q_OBJECT

public:
    virtual ~AParticleGun(){}

    virtual bool Init() = 0;             //called before first use
    virtual void ReleaseResources() {}   //called after end of operation
    virtual bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) = 0;

    //virtual void RemoveParticle(int particleId) = 0; //should NOT be used to remove one of particles in use! use onIsPareticleInUse first
    //virtual bool IsParticleInUse(int particleId, QString& SourceNames) const = 0;

    virtual void writeToJson(QJsonObject &json) const = 0;
    virtual bool readFromJson(const QJsonObject &json) = 0;

    virtual void SetStartEvent(int) {} // for 'from file' generator


    const QString & GetErrorString() const {return ErrorString;}
    void            SetErrorString(const QString& str) {ErrorString = str;}

    bool            IsAbortRequested() const {return bAbortRequested;}

public slots:
    virtual void abort() {bAbortRequested = true;}

protected:
    QString ErrorString;
    bool bAbortRequested = false;
};

#endif // APARTICLEGUN_H
