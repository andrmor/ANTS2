#include "arecipe.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include "asensor.h"
#include "arepository.h"
#include "ainstruction.h"

namespace LRF {

extern const ARecipeID invalid_recipe_id(-1);

bool ARecipe::addInstruction(AInstructionID instruction)
{
  if(versions.empty()) {
    instructions.push_back(instruction);
    return true;
  }
  return false;
}

bool ARecipe::hasInstruction(AInstructionID instruction) const
{
  auto iter = std::find(instructions.begin(), instructions.end(), instruction);
  return iter != instructions.end();
}

ARecipe::Version &ARecipe::getVersion(ARecipeVersionID vid)
{
  if(vid == ARecipeVersionID(-1))
    return versions.back();

  for(Version &v : versions)
    if(v.id == vid)
      return v;
  return versions.front();
}

const ARecipe::Version &ARecipe::getVersion(ARecipeVersionID vid) const
{
  if(vid == ARecipeVersionID(-1))
    return versions.back();

  for(const Version &v : versions)
    if(v.id == vid)
      return v;
  return versions.front();
}

bool ARecipe::hasVersion(ARecipeVersionID vid) const
{
  if(vid == ARecipeVersionID(-1))
    return !versions.empty();

  for(const Version &v : versions)
    if(v.id == vid)
      return true;
  return false;
}

void ARecipe::addVersion(ARecipeVersionID vid, ASensorGroup sensors)
{
  versions.push_back(ARecipeVersion(vid, "Version "+std::to_string(vid.val()), std::move(sensors)));
}

bool ARecipe::removeVersion(ARecipeVersionID vid)
{
  auto iter = std::find_if(versions.begin(), versions.end(),
            [&vid](const Version &v) { return v.id == vid; });
  if(iter != versions.end()) {
    versions.erase(iter);
    return true;
  }
  return false;
}

void ARecipe::remapInstructionIDs(const std::map<AInstructionID, AInstructionID> &map)
{
  for(auto &key_val : map)
    for(auto &old_iid : instructions)
      if(old_iid == key_val.first)
        old_iid = key_val.second;
}

}
