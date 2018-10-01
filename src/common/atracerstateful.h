#ifndef ATRACERSTATEFUL_H
#define ATRACERSTATEFUL_H

class TRandom2;
class QScriptEngine;
class QObject;
class AMaterialParticleCollection;

#include <QVector>

//random generator is external
//script engine is owned by this class, but created only if needed (there are script overrides)

class ATracerStateful
{
public:
    ~ATracerStateful();

    void evaluateScript(const QString& Script);

    void registerAllInterfaceObjects(AMaterialParticleCollection* MPcollection);

    //called by MPcollection
    void registerInterfaceObject(QObject* interfaceObj);

    TRandom2 * RandGen = 0;
    QScriptEngine * ScriptEngine = 0;

private:
    QVector<QObject*> interfaces;
};

#endif // ATRACERSTATEFUL_H
