#ifndef ATRACERSTATEFUL_H
#define ATRACERSTATEFUL_H

#include <QString>

class TRandom2;
class QScriptEngine;
class QObject;
class AMaterialParticleCollection;
class AOpticalOverrideScriptInterface;

//random generator is external
//script engine is owned by this class, but created only if needed (there are script overrides)

class ATracerStateful
{
public:
    ~ATracerStateful();

    void evaluateScript(const QString& Script);

    void generateScriptInfrastructureIfNeeded(AMaterialParticleCollection* MPcollection);

    //called by MPcollection
    void registerInterfaceObject(AOpticalOverrideScriptInterface *interfaceObj);

    TRandom2 * RandGen = 0;
    QScriptEngine * ScriptEngine = 0;
    AOpticalOverrideScriptInterface* interfaceObject = 0;
};

#endif // ATRACERSTATEFUL_H
