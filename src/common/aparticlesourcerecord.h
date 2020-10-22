#ifndef APARTICLESOURCERECORD_H
#define APARTICLESOURCERECORD_H

#include <QVector>
#include <QString>

class TH1D;
class QJsonObject;
class AMaterialParticleCollection;
class TRandom2;

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

    TH1D *  spectrum = nullptr; //energy spectrum

    GunParticleStruct * clone() const;

    double  generateEnergy(TRandom2 *RandGen) const;
    bool    loadSpectrum(const QString& fileName);

    void writeToJson(QJsonObject & json, const AMaterialParticleCollection & MpCollection) const;
    bool readFromJson(const QJsonObject & json, AMaterialParticleCollection & MpCollection);

};

struct AParticleSourceRecord
{
    ~AParticleSourceRecord(); //deletes records in dynamic GunParticles

    QString name = "No_name";
    int shape = 0;    // change to enum
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

    //time
    int    TimeAverageMode = 0;
    double TimeAverage = 0;
    double TimeAverageStart = 0;
    double TimeAveragePeriod = 10.0;
    int    TimeSpreadMode = 0;
    double TimeSpreadSigma = 50.0;
    double TimeSpreadWidth = 100.0;

    //particles
    QVector<GunParticleStruct*> GunParticles;

    AParticleSourceRecord * clone() const;

    void writeToJson(QJsonObject & json, const AMaterialParticleCollection & MpCollection) const;
    bool readFromJson(const QJsonObject & json, AMaterialParticleCollection & MpCollection);

    const QString getShapeString() const;

    const QString checkSource(const AMaterialParticleCollection & MpCollection) const; //return error description if error found

    void updateLimitedToMat(const AMaterialParticleCollection & MpCollection);

    //runtime properties
    int  LimitedToMat; //automatically calculated if LimtedToMatName matches a material
    bool bLimitToMat = false;

private:
    void clearGunParticles();
};


#endif // APARTICLESOURCERECORD_H
