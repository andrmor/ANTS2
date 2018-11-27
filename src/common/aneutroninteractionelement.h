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

class TH1D;
class ADecayScenario
{
public:
    double Branching;

    //full model
    QVector<AAbsorptionGeneratedParticle> GeneratedParticles;

    //simplified model
    enum DirectDepoSelected { NotActive = 0, ConstModel = 1, GaussModel = 2, CustomDistModel = 3};
    DirectDepoSelected eDirectDepo = NotActive; //if any other than NotActive is selected, GeneratedParticles are ignored!
    double DirectAverage = 1000.0; // keV
    double DirectSigma = 10.0; //keV
    QVector<double> DirectCustomEn;
    QVector<double> DirectCustomProb;
    TH1D* DirectCustomDist = 0;  //run-time property!

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

    void updateRuntimeProperties();
};

#endif // ANEUTRONINTERACTIONELEMENT_H
