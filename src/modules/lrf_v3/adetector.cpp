#include "adetector.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include "asensor.h"
#include "arepository.h"
#include "ainstruction.h"

namespace LRF {

/*
QJsonObject ADetector::writeJson() const
{
  QJsonObject detector;

  QJsonArray jsoninstructions;
  for(auto i : instructions)
    jsoninstructions.append(r->toJson());
  detector["Instructions"] = jsoninstructions;

  return detector;
}*/

bool ADetector::addInstruction(AInstructionID instruction)
{
  if(versions.empty()) {
    qDebug()<<"Added instruction"<<instruction.val();
    instructions.push_back(instruction);
    return true;
  }
  return false;
}

bool ADetector::removeInstruction(AInstructionID instruction)
{
  if(versions.empty()) {
    auto iter = std::find(instructions.begin(), instructions.end(), instruction);
    if(iter != instructions.end()) {
      instructions.erase(iter);
      return true;
    }
  }
  return false;
}

bool ADetector::hasInstruction(AInstructionID instruction)
{
  auto iter = std::find(instructions.begin(), instructions.end(), instruction);
  return iter != instructions.end();
}

const ADetector::type_version *ADetector::getVersion(const std::string &version) const
{
  auto iter = std::find_if(versions.begin(), versions.end(),
            [&version](const type_version &v) { return v.first == version; });
  return iter != versions.end() ? &*iter : nullptr;
}

bool ADetector::removeVersion(const std::string &version)
{
  auto iter = std::find_if(versions.begin(), versions.end(),
            [&version](const type_version &v) { return v.first == version; });
  if(iter != versions.end()) {
    versions.erase(iter);
    return true;
  }
  return false;
}

void ADetector::clearHistory()
{
  versions.clear();
  //current = ASensorGroupID(-1);
}

#if 0
bool ADetector::setCurrentVersion(const std::string &version)
{
  for(size_t i = 0; i < versions.size(); ++i) {
    if(versions[i].first == version) {
      current = ASensorGroupID(i);
      return true;
    }
  }
  return false;
}
#endif

bool ADetector::update(const ARepository *repo, const AInstructionInput &data)
{
  ASensorGroup sensors;
  for(auto &iid : instructions) {
    const AInstruction *instruction = repo->get(iid);
    if(!instruction) return false;
    instruction->apply(sensors, data);
  }
  auto name = "Version "+std::to_string(versions.size());
  versions.push_back(std::make_pair(std::move(name), std::move(sensors)));
  //current = ASensorGroupID(versions.size()-1);
  qDebug()<<"Created version"<<name.data();
  return true;
}

}
