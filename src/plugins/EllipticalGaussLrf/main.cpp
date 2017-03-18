#include <memory>

#include "ellipticalgausslrf_global.h"
#include "alrftypemanagerinterface.h"
#include "idclasses.h"
#include "type.h"

extern "C" ELLIPTICALGAUSSLRFSHARED_EXPORT void Setup(LRF::ALrfTypeManagerInterface &manager) {
  LRF::ALrfTypeID tid = manager.registerLrfType(std::unique_ptr<Type>(new Type));
  manager.registerToGroup(tid, "Example plugins");
}
