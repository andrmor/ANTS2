#ifndef APARTICLEGENERATORINTERFACE_H
#define APARTICLEGENERATORINTERFACE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVector>

class AParticleRecord;
class AMaterialParticleCollection;
class TRandom2;

class AParticleGenerator_SI : public AScriptInterface
{
    Q_OBJECT

public:
    AParticleGenerator_SI(const AMaterialParticleCollection & MpCollection, TRandom2 * RandGen);

    void configure(QVector<AParticleRecord*> * GeneratedParticles);

public slots:
    void AddParticle(int type, double energy, double x, double y, double z, double i, double k, double j, double time = 0);
    void AddParticleIsotropic(int type, double energy, double x, double y, double z, double time = 0);

private:
    const AMaterialParticleCollection & MpCollection;
    TRandom2 * RandGen = 0;                                 //external
    QVector<AParticleRecord*> * GP = 0;                     //external

};

#endif // APARTICLEGENERATORINTERFACE_H
