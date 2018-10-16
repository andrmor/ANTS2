#ifndef APARTICLESOURCERECORD_H
#define APARTICLESOURCERECORD_H

#include <QVector>
#include <QString>

class TH1D;
class QJsonObject;
class AMaterialParticleCollection;

struct GunParticleStruct
{
    ~GunParticleStruct(); //deletes spectrum

    int     ParticleId = 0;
    double  StatWeight = 1.0;
    bool    bUseFixedEnergy = true;
    double  energy = 100.0; //in keV
    QString PreferredUnits = "keV";
    bool    Individual = true; // true = individual particle; false = linked
    int     LinkedTo = 0; // index of the "parent" particle this one is following
    double  LinkingProbability = 0;  //probability to be emitted after the parent particle
    bool    LinkingOppositeDir = false; // false = random direction; otherwise particle is emitted in the opposite direction in respect to the LinkedTo particle

    TH1D*   spectrum = 0; //energy spectrum

    GunParticleStruct * clone() const;

    double  generateEnergy() const;
    bool    loadSpectrum(const QString& fileName);

    void writeToJson(QJsonObject & json, const AMaterialParticleCollection & MpCollection) const;
    bool readFromJson(const QJsonObject & json, AMaterialParticleCollection & MpCollection);

};

struct AParticleSourceRecord
{
    ~AParticleSourceRecord(); //deletes records in dynamic GunParticles

    QString name = "Underfined";
    //source type (geometry)
    int index = 0; //shape
    //position
    double X0 = 0;
    double Y0 = 0;
    double Z0 = 0;
    //orientation
    double Phi = 0;
    double Theta = 0;
    double Psi = 0;
    //size
    double size1 = 10.0;
    double size2 = 10.0;
    double size3 = 10.0;
    //collimation
    double CollPhi = 0;
    double CollTheta = 0;
    double Spread = 45.0;

    //limit to material
    bool DoMaterialLimited = false;
    QString LimtedToMatName;

    //Relative activity
    double Activity = 1.0;

    //particles
    QVector<GunParticleStruct*> GunParticles;

    AParticleSourceRecord * clone() const;

    void writeToJson(QJsonObject & json, const AMaterialParticleCollection & MpCollection) const;
    bool readFromJson(const QJsonObject & json, AMaterialParticleCollection & MpCollection);

    const QString getShapeString() const;

    //local variables, used in tracking, calculated autonatically, not to be loaded/saved!
    int LimitedToMat; //automatically calculated if LimtedToMatName matches a material
    bool fLimit = false;

private:
    void clearGunParticles();
};


#endif // APARTICLESOURCERECORD_H
