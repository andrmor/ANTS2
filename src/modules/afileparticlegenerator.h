#ifndef AFILEPARTICLEGENERATOR_H
#define AFILEPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>
#include <QFile>
#include <QRegularExpression>
#include <QVector>
#include <QJsonObject>
#include <QDateTime>

class AMaterialParticleCollection;
class QTextStream;

class AFileParticleGenerator : public AParticleGun
{
public:
    AFileParticleGenerator(const AMaterialParticleCollection& MpCollection);
    virtual ~AFileParticleGenerator(){}

    void          SetFileName(const QString &fileName);
    const QString GetFileName() const {return FileName;}

    virtual bool Init() override;               //called before first use
    virtual void ReleaseResources() override;   //called after end of operation
    virtual bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;

    virtual const QString CheckConfiguration() const override; //check consistency of the configuration

    virtual void RemoveParticle(int particleId) override; //cannot be used for this class
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

    virtual void writeToJson(QJsonObject& json) const override;
    virtual bool readFromJson(const QJsonObject& json) override;

    virtual void SetStartEvent(int startEvent) override;

    void InvalidateFile(); //signals that the file has to be inspected again by running Init()
    bool IsValidated() const;

    //public file inspect results
    int NumEventsInFile = 0;          //saved
    int statNumMultipleEvents = 0;
    QVector<int> statParticleQuantity;

private:
    const AMaterialParticleCollection & MpCollection;
    QString FileName;
    QFile File;
    QTextStream* Stream = 0;

    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'

    int RegisteredParticleCount = -1; //saved - used in validity check
    QDateTime FileLastModified;       //saved - used in validity check


private:
    void clearFileStat();

};

#endif // AFILEPARTICLEGENERATOR_H
