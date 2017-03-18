#include "alrftypemanager.h"

#include "alrf.h"
#include "acommonfunctions.h"

#include <QDebug>

namespace LRF {

ALrfTypeManager ALrfTypeManager::global_instance;

ALrfTypeID ALrfTypeManager::typeIdFromName(const std::string &name) const
{
  auto type_id = name_to_id.find(name);
  if(type_id == name_to_id.end()) {
    qDebug()<<"Warning: trying to access unregistered LRF type"<<name.data();
    return ALrfTypeID(-1);
  }
  return type_id->second;
}

const ALrfType *ALrfTypeManager::getType(ALrfTypeID id) const
{
  if(id == ALrfTypeID(-1))
    return nullptr;
  else
    return lrf_types[id.val()].get();
}

ALrfTypeID ALrfTypeManager::registerLrfType(std::unique_ptr<ALrfType> type) {
  const std::string &name = type->name();
  if(name_to_id.find(name) != name_to_id.end()) {
    qDebug()<<"Attempting to register already existing "<<name.data()<<" LRF type";
    return ALrfTypeID(-1);
  }
  type->id_ = ALrfTypeID(lrf_types.size());
  name_to_id[name] = type->id_;
  lrf_types.push_back(std::move(type));
  return lrf_types.back()->id_;
}

bool ALrfTypeManager::registerAlias(ALrfTypeID type_id, const std::string &alias)
{
  if(name_to_id.find(alias) != name_to_id.end()) {
    qDebug()<<"Attempting to register alias of already existing "<<alias.data()<<" LRF type";
    return false;
  }
  const ALrfType *type = getType(type_id);
  if(!type) {
    qDebug()<<"Attempting to register alias of non-existing "<<type_id.val()<<" LRF type";
    return false;
  }
  name_to_id[alias] = type->id();
  return true;
}

bool ALrfTypeManager::registerToGroup(ALrfTypeID type_id, const std::string &group)
{
  type_groups[group].push_back(type_id);
  return true;
}

std::vector<std::string> ALrfTypeManager::getGroups() const
{
  return getMapKeys(type_groups);
}

const std::vector<ALrfTypeID> *ALrfTypeManager::getTypesInGroup(const std::string &group) const
{
  auto it = type_groups.find(group);
  if(it != type_groups.end())
    return &it->second;
  return nullptr;
}

const ALrfType &ALrfTypeManager::getTypeFast(ALrfTypeID id) const
{
  return *lrf_types[id.val()].get();
}

QJsonObject ALrfTypeManager::lrfToJson(const ALrf *lrf) const
{
  const ALrfType *type = lrf_types[lrf->type().val()].get();
  QJsonObject json = type->lrfToJson(lrf);
  //Give a chance for the type to give its own name (good for aliases)
  if(!json["type"].isString())
    json["type"] = type->name().data();
  return json;
}

ALrf *ALrfTypeManager::lrfFromJson(const QJsonObject &json) const
{
  std::string type_name = json["type"].toString().toLocal8Bit().data();
  const ALrfType *type = getType(type_name);
  return type ? type->lrfFromJson(json) : nullptr;
}

ALrf *ALrfTypeManager::lrfFromJson(const QJsonObject &json, ALrfTypeID tid) const
{
  const ALrfType *type = getType(tid);
  return type ? type->lrfFromJson(json) : nullptr;
}

} //namespace LRF
