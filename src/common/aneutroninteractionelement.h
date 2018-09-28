#ifndef ANEUTRONINTERACTIONELEMENT_H
#define ANEUTRONINTERACTIONELEMENT_H

#include <QString>
#include <QVector>

class QJsonObject;
class AMaterialParticleCollection;

class AAbsorptionGeneratedParticle
{
public:
    int ParticleId;
    double Energy;    // in keV
    bool bOpositeDirectionWithPrevious;

    AAbsorptionGeneratedParticle(int ParticleId, double Energy, bool bOpositeDirectionWithPrevious) :
    ParticleId(ParticleId), Energy(Energy), bOpositeDirectionWithPrevious(bOpositeDirectionWithPrevious) {}
    AAbsorptionGeneratedParticle() : ParticleId(0), Energy(0), bOpositeDirectionWithPrevious(false) {}

    void writeToJson(QJsonObject& json, AMaterialParticleCollection *MpCollection) const;
    const QJsonObject writeToJson(AMaterialParticleCollection *MpCollection) const;
    void readFromJson(const QJsonObject& json, AMaterialParticleCollection *MpCollection);
};

class ADecayScenario
{
public:
    double Branching;
    QVector<AAbsorptionGeneratedParticle> GeneratedParticles;

    ADecayScenario(double Branching) : Branching(Branching) {}
    ADecayScenario() : Branching(1.0) {}

    void writeToJson(QJsonObject& json, AMaterialParticleCollection *MpCollection) const;
    const QJsonObject writeToJson(AMaterialParticleCollection *MpCollection) const;
    void readFromJson(const QJsonObject& json, AMaterialParticleCollection *MpCollection);
};

class ANeutronInteractionElement  // basic class for capture and elastic scattering
{
public:
    ANeutronInteractionElement(QString IsotopeSymbol, int Mass, double MolarFraction) :
        Name(IsotopeSymbol), Mass(Mass), MolarFraction(MolarFraction) {}
    ANeutronInteractionElement() : Name("Undefined"), Mass(777), MolarFraction(0) {}

    QString Name;
    int Mass;
    double MolarFraction;

    QVector<double> Energy;
    QVector<double> CrossSection;

    QString CSfileHeader;

    QVector<ADecayScenario> DecayScenarios;  // only for absorption

    void writeToJson(QJsonObject& json, AMaterialParticleCollection *MpCollection) const;
    const QJsonObject writeToJson(AMaterialParticleCollection *MpCollection) const;
    void readFromJson(const QJsonObject& json, AMaterialParticleCollection *MpCollection);

    void readScenariosFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection);
};

#endif // ANEUTRONINTERACTIONELEMENT_H
