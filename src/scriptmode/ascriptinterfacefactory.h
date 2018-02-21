#ifndef ASCRIPTINTERFACEFACTORY_H
#define ASCRIPTINTERFACEFACTORY_H

#include "ascriptinterface.h"
#include "coreinterfaces.h"
#include "interfacetoglobscript.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "ainterfacetowebsocket.h"
#include "ainterfacetomessagewindow.h"

#include <QObject>

class AScriptInterfaceFactory
{
public:    
    static QObject* makeCopy(const QObject* other)
    {
        const AInterfaceToCore* core = dynamic_cast<const AInterfaceToCore*>(other);
        if (core) return new AInterfaceToCore(*core);

        const AInterfaceToMath* math = dynamic_cast<const AInterfaceToMath*>(other);
        if (math) return new AInterfaceToMath(*math);

        const AInterfaceToConfig* config = dynamic_cast<const AInterfaceToConfig*>(other);
        if (config) return new AInterfaceToConfig(*config);

        const AInterfaceToMinimizerScript* mini = dynamic_cast<const AInterfaceToMinimizerScript*>(other);
        if (mini) return new AInterfaceToMinimizerScript(*mini);

        const AInterfaceToHist* hist = dynamic_cast<const AInterfaceToHist*>(other);
        if (hist) return new AInterfaceToHist(*hist);

        const AInterfaceToGraph* graph = dynamic_cast<const AInterfaceToGraph*>(other);
        if (graph) return new AInterfaceToGraph(*graph);

        const AInterfaceToMessageWindow* msg = dynamic_cast<const AInterfaceToMessageWindow*>(other);
        if (msg) return new AInterfaceToMessageWindow(*msg);

#ifdef SPEECH
        const AInterfaceToSpeech* speech = dynamic_cast<const AInterfaceToSpeech*>(other);
        if (speech) return new AInterfaceToSpeech(*speech);
#endif

        return 0;
    }
};

#endif // ASCRIPTINTERFACEFACTORY_H
