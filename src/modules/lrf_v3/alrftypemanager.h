#ifndef ALRFTYPEMANAGER_H
#define ALRFTYPEMANAGER_H

#include <vector>
#include <memory>
#include <map>

#include "alrftypemanagerinterface.h"
#include "alrftype.h"

class QJsonObject;
class QWidget;

namespace LRF {

///
/// \brief The ALrfTypeManager class
///
class ALrfTypeManager : public ALrfTypeManagerInterface {
  std::vector<std::unique_ptr<const ALrfType>> lrf_types;
  std::map<std::string, ALrfTypeID> name_to_id;
  std::map<std::string, std::vector<ALrfTypeID>> type_groups;

  //I don't see a need to have multiple ALrfTypeManager
  static ALrfTypeManager global_instance;
  ALrfTypeManager() {}
public:
  static ALrfTypeManager &instance() { return global_instance; }

  ALrfTypeID registerLrfType(std::unique_ptr<ALrfType> type) override;
  bool registerAlias(ALrfTypeID type_id, const std::string &alias) override;
  bool registerToGroup(ALrfTypeID type_id, const std::string &group) override;

  std::vector<std::string> getGroups() const;
  const std::vector<ALrfTypeID> *getTypesInGroup(const std::string &group) const;

  bool isEmpty() const { return lrf_types.empty(); }
  size_t size() const { return lrf_types.size(); }
  ALrfTypeID last() const { return ALrfTypeID(lrf_types.size()); }

  ALrfTypeID typeIdFromName(const std::string &name) const;
  const ALrfType *getType(ALrfTypeID id) const;

  ///
  /// \brief getTypeFast assumes that id < last() (note that id is unsigned)
  ///
  const ALrfType &getTypeFast(ALrfTypeID id) const;

  const ALrfType *getType(const std::string &name) const
  { return getType(typeIdFromName(name)); }

  //Provide access to LRF save/load methods, to enforce proper ["type"] match.
  ALrf *lrfFromJson(const QJsonObject &json) const;
  ALrf *lrfFromJson(const QJsonObject &json, ALrfTypeID tid) const;
  QJsonObject lrfToJson(const ALrf *lrf) const;
};

}


#endif // ALRFTYPEMANAGER_H
