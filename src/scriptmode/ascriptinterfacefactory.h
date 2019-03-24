#ifndef ASCRIPTINTERFACEFACTORY_H
#define ASCRIPTINTERFACEFACTORY_H

#include "ascriptinterface.h"
#include "acore_si.h"
#include "amath_si.h"
#include "interfacetoglobscript.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "aweb_si.h"
#include "aconfig_si.h"

#ifdef GUI
#include "amsg_si.h"
#endif

#include <QObject>

class AScriptInterfaceFactory
{
public:    
    static AScriptInterface* makeCopy(const AScriptInterface* other)
    {
        const ACore_SI* core = dynamic_cast<const ACore_SI*>(other);
        if (core) return new ACore_SI(*core);

        const AMath_SI* math = dynamic_cast<const AMath_SI*>(other);
        if (math) return new AMath_SI(*math);

        const AConfig_SI* config = dynamic_cast<const AConfig_SI*>(other);
        if (config) return new AConfig_SI(*config);

        const AMini_JavaScript_SI* mini = dynamic_cast<const AMini_JavaScript_SI*>(other);
        if (mini) return new AMini_JavaScript_SI(*mini);

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
        const AMsg_SI* msg = dynamic_cast<const AMsg_SI*>(other);
        if (msg) return new AMsg_SI(*msg);
#endif
        const AWeb_SI* web = dynamic_cast<const AWeb_SI*>(other);
        if (web) return new AWeb_SI(*web);

        return 0;
    }
};

#endif // ASCRIPTINTERFACEFACTORY_H
