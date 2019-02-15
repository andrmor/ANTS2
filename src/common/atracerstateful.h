#ifndef ATRACERSTATEFUL_H
#define ATRACERSTATEFUL_H

#include <QString>

class TRandom2;
class QScriptEngine;
class QObject;
class AMaterialParticleCollection;
class AOpticalOverrideScriptInterface;
class AMathScriptInterface;

//random generator is external
//script engine is owned by this class, but created only if needed (there are script overrides)

class ATracerStateful
{
public:
    ATracerStateful(TRandom2* RandGen);
    ~ATracerStateful();

    void evaluateScript(const QString& Script);

    void generateScriptInfrastructureIfNeeded(const AMaterialParticleCollection* MPcollection); //called by PhotonTracer (one per each thread!)

    void generateScriptInfrastructure(const AMaterialParticleCollection *MPcollection); //can be use from outside to force generation (e.g. interface tester); RandGen should already be set!

    void abort();

    TRandom2 * RandGen = 0;                                 //external
    QScriptEngine * ScriptEngine = 0;                       //local
    AOpticalOverrideScriptInterface* overrideInterface = 0; //local
    AMathScriptInterface* mathInterface = 0;                //local
};

#endif // ATRACERSTATEFUL_H
