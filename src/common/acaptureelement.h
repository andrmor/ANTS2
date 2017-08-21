#ifndef ACAPTUREELEMENT_H
#define ACAPTUREELEMENT_H

#include <QString>
#include <QVector>

class QJsonObject;
class AMaterialParticleCollection;

class ACaptureGeneratedParticle
{
public:
    int ParticleId;
    double Energy;    // in keV
    bool bOpositeDirectionWithPrevious;

    ACaptureGeneratedParticle(int ParticleId, double Energy, bool bOpositeDirectionWithPrevious) :
    ParticleId(ParticleId), Energy(Energy), bOpositeDirectionWithPrevious(bOpositeDirectionWithPrevious) {}
    ACaptureGeneratedParticle() : ParticleId(0), Energy(0), bOpositeDirectionWithPrevious(false) {}

    void writeToJson(QJsonObject& json, AMaterialParticleCollection *MpCollection) const;
    const QJsonObject writeToJson(AMaterialParticleCollection *MpCollection) const;
    void readFromJson(const QJsonObject& json, AMaterialParticleCollection *MpCollection);
};

class ACaptureReaction
{
public:
    double Branching;
    QVector<ACaptureGeneratedParticle> GeneratedParticles;

    ACaptureReaction() : Branching(1.0) {}

    void writeToJson(QJsonObject& json, AMaterialParticleCollection *MpCollection) const;
    const QJsonObject writeToJson(AMaterialParticleCollection *MpCollection) const;
    void readFromJson(const QJsonObject& json, AMaterialParticleCollection *MpCollection);
};

class ACaptureElement
{
public:
    QString Name;
    int Mass;
    double MolarFraction;

    QVector<double> Energy;
    QVector<double> CrossSection;

    QVector<ACaptureReaction> Reactions;

    ACaptureElement(QString IsotopeName, int Mass, double MolarFraction) :
        Name(IsotopeName), Mass(Mass), MolarFraction(MolarFraction) {Reactions.resize(1);}
    ACaptureElement() : Name("Undefined"), Mass(777), MolarFraction(0) {Reactions.resize(1);}

    void writeToJson(QJsonObject& json, AMaterialParticleCollection *MpCollection) const;
    const QJsonObject writeToJson(AMaterialParticleCollection *MpCollection);
    void readFromJson(QJsonObject& json, AMaterialParticleCollection *MpCollection);
};

#endif // ACAPTUREELEMENT_H
