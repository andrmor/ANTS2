#ifndef ALRFTYPEMANAGERINTERFACE_H
#define ALRFTYPEMANAGERINTERFACE_H

#include <memory>
#include <string>

#include "idclasses.h"

namespace LRF {

class ALrfType;

class ALrfTypeManagerInterface
{
public:
  virtual ALrfTypeID registerLrfType(std::unique_ptr<ALrfType> type) = 0;
  virtual bool registerAlias(ALrfTypeID type_id, const std::string &alias) = 0;
  virtual bool registerToGroup(ALrfTypeID type_id, const std::string &group) = 0;
};

}

#endif // ALRFTYPEMANAGERINTERFACE_H
