#ifndef APARTICLEGENERATORINTERFACE_H
#define APARTICLEGENERATORINTERFACE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVector>
#include <QVariantList>
//#include <QVariant>

class AParticleRecord;
class AMaterialParticleCollection;
class TRandom2;

class AParticleGenerator_SI : public AScriptInterface
{
    Q_OBJECT

public:
    AParticleGenerator_SI(const AMaterialParticleCollection & MpCollection, TRandom2 * RandGen, int ThreadId, const int * NumRunningThreads);

    void configure(QVector<AParticleRecord*> * GeneratedParticles);

public slots:
    void AddParticle(int type, double energy, double x, double y, double z, double i, double k, double j, double time = 0);
    void AddParticleIsotropic(int type, double energy, double x, double y, double z, double time = 0);

    void         StoreVariables(QVariantList array);
    QVariantList RetrieveVariables();

    int GetThreadId() {return ThreadId;}
    int GetNumThreads() {return *NumRunningThreads;}

private:
    const AMaterialParticleCollection & MpCollection;
    TRandom2 * RandGen = 0;                                 //external
    int ThreadId = 0;
    const int * NumRunningThreads;
    QVector<AParticleRecord*> * GP = 0;                     //external

    QVariantList StoredData;
};

#endif // APARTICLEGENERATORINTERFACE_H
