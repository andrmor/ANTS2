#ifndef ASCRIPTINTERFACEFACTORY_H
#define ASCRIPTINTERFACEFACTORY_H

#include "ascriptinterface.h"
#include "acorescriptinterface.h"
#include "amathscriptinterface.h"
#include "interfacetoglobscript.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "ainterfacetowebsocket.h"
#ifdef GUI
#include "ainterfacetomessagewindow.h"
#endif

#include <QObject>

class AScriptInterfaceFactory
{
public:    
    static AScriptInterface* makeCopy(const AScriptInterface* other)
    {
        const ACoreScriptInterface* core = dynamic_cast<const ACoreScriptInterface*>(other);
        if (core) return new ACoreScriptInterface(*core);

        const AMathScriptInterface* math = dynamic_cast<const AMathScriptInterface*>(other);
        if (math) return new AMathScriptInterface(*math);

        const AInterfaceToConfig* config = dynamic_cast<const AInterfaceToConfig*>(other);
        if (config) return new AInterfaceToConfig(*config);

        const AInterfaceToMinimizerJavaScript* mini = dynamic_cast<const AInterfaceToMinimizerJavaScript*>(other);
        if (mini) return new AInterfaceToMinimizerJavaScript(*mini);

        const AInterfaceToData* events = dynamic_cast<const AInterfaceToData*>(other);
        if (events) return new AInterfaceToData(*events);

        const AInterfaceToLRF* lrf = dynamic_cast<const AInterfaceToLRF*>(other);
        if (lrf) return new AInterfaceToLRF(*lrf);

        const AInterfaceToPMs* pms = dynamic_cast<const AInterfaceToPMs*>(other);
        if (pms) return new AInterfaceToPMs(*pms);

        const AInterfaceToHist* hist = dynamic_cast<const AInterfaceToHist*>(other);
        if (hist) return new AInterfaceToHist(*hist);

        const AInterfaceToGraph* graph = dynamic_cast<const AInterfaceToGraph*>(other);
        if (graph) return new AInterfaceToGraph(*graph);
#ifdef GUI
        const AInterfaceToMessageWindow* msg = dynamic_cast<const AInterfaceToMessageWindow*>(other);
        if (msg) return new AInterfaceToMessageWindow(*msg);
#endif
        const AInterfaceToWebSocket* web = dynamic_cast<const AInterfaceToWebSocket*>(other);
        if (web) return new AInterfaceToWebSocket(*web);

        return 0;
    }
};

#endif // ASCRIPTINTERFACEFACTORY_H
