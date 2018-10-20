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
    virtual QVector<AGeneratedParticle>* GenerateEvent();

    virtual const QString CheckConfiguration() const override; //check consistency of the configuration

    virtual void RemoveParticle(int particleId) override; //cannot be used for this class
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

    virtual void writeToJson(QJsonObject& json) const override;
    virtual bool readFromJson(const QJsonObject& json) override;

    virtual void SetStartEvent(int startEvent) override;

    void InvalidateFile(); //signals that the file has to be inspected again

    //public file inspect results
    int statNumEvents = 0;
    int statNumMultipleEvents = 0;
    QVector<int> statParticleQuantity;

private:
    const AMaterialParticleCollection & MpCollection;
    QString FileName;
    QFile File;
    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
    int RegisteredParticleCount = -1;

    QTextStream* Stream = 0;
    QDateTime FileLastModified;

private:
    void clearFileStat();

};

#endif // AFILEPARTICLEGENERATOR_H
