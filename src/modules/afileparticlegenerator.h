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
    virtual ~AFileParticleGenerator();

    void          SetFileName(const QString &fileName);
    const QString GetFileName() const {return FileName;}

    virtual bool Init() override;               //has to be called before first use of GenerateEvent()
    virtual void ReleaseResources() override;
    virtual bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;

    virtual void RemoveParticle(int) override {} //cannot be used for this class
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

    virtual void writeToJson(QJsonObject& json) const override;
    virtual bool readFromJson(const QJsonObject& json) override;

    virtual void SetStartEvent(int startEvent) override;

    void InvalidateFile();    //forces the file to be inspected again during next call of Init()
    bool IsValidated() const;

    const QString GetEventRecords(int fromEvent, int toEvent) const;

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
