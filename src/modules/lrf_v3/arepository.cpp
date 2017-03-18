#include "arepository.h"

#include <set>
#include <algorithm>
#include <utility>

#include <QDebug>
#include <QApplication> //for qApp->processEvents();

#include "alrf.h"
#include "alrftypemanager.h"
#include "ainstructioninput.h"
#include "common/acommonfunctions.h"

namespace LRF {

class ARepositorySave {
  QVector<ARecipeID> saved_recipes;
  QVector<std::pair<ARecipeID, ARecipeVersionID>> saved_versions;
  QJsonArray recipes;
  const ARepository *repo;

  QJsonArray instructionsToJson(const ARecipe &recipe, QVector<std::pair<ARecipeID, ARecipeVersionID>> &dependencies)
  {
    QJsonArray recipe_instructions;
    int count = recipe.getInstructionCount();
    for(int i = 0; i < count; i++) {
      AInstructionID iid = recipe.getInstruction(i);
      if(!repo->hasInstruction(iid)) {
        qDebug()<<"ARepositorySave::saveRecipe(ARecipeID rid): Recipe has instruction not present in repository. Should never happen!";
        continue;
      }
      const AInstruction &instruction = repo->getInstruction(iid);
      recipe_instructions.append(instruction.toJson());
      dependencies << QVector<std::pair<ARecipeID, ARecipeVersionID>>::fromStdVector(instruction.getDependencies());
    }
    return recipe_instructions;
  }

  QJsonObject versionToJson(const ARecipeVersion &version)
  {
    QJsonObject json_version;
    json_version["name"] = QString::fromStdString(version.name);
    json_version["id"] = (int)version.id.val();
    json_version["sensors"] = version.sensors.toJson();
    json_version["creation time"] = QString::fromStdString(version.creation_time);
    json_version["comment"] = QString::fromStdString(version.comment);
    return json_version;
  }

public:
  ARepositorySave(const ARepository *repo) : repo(repo) {}

  void saveRecipe(ARecipeID rid, bool recursive = true) {
    //If recipe doesn't exist or was already saved, do nothing.
    if(!repo->hasRecipe(rid) || saved_recipes.contains(rid)) return;

    //If we have already saved some versions of this recipe, remove them
    for(int i = 0; i < recipes.size(); i++) {
      if(recipes[i].toObject()["id"].toInt() == (int)rid.val())
        recipes.removeAt(i--);
    }
    std::remove_if(saved_versions.begin(), saved_versions.end(),
                   [rid](const std::pair<ARecipeID, ARecipeVersionID> &rid_vid) { return rid == rid_vid.first; });

    //Get the recipe
    const ARecipe &recipe = repo->getRecipe(rid);

    //Save the instructions, and get their dependencies
    QVector<std::pair<ARecipeID, ARecipeVersionID>> dependencies;
    QJsonArray recipe_instructions = instructionsToJson(recipe, dependencies);

    //Save the history
    QJsonArray recipe_history;
    for(const ARecipeVersion &version : recipe.history())
      recipe_history.append(versionToJson(version));

    //Wrap up the recipe
    QJsonObject json_recipe;
    json_recipe["name"] = QString::fromStdString(recipe.name());
    json_recipe["instructions"] = recipe_instructions;
    json_recipe["history"] = recipe_history;
    json_recipe["id"] = (int)rid.val();

    //Store it, and signal that
    recipes.append(json_recipe);
    saved_recipes.push_back(rid);

    if(recursive) {
      for(const auto &dep : dependencies)
        saveVersion(dep.first, dep.second);
    }
  }

  void saveVersion(ARecipeID rid, ARecipeVersionID vid, bool recursive = true) {
    //If version doesn't exist or recipe-version was already saved, do nothing.
    if(!repo->hasVersion(rid, vid) || saved_recipes.contains(rid) || saved_versions.contains(std::make_pair(rid, vid)))
      return;

    //Get the recipe, and possibly fix vid
    const ARecipe &recipe = repo->getRecipe(rid);
    if(vid == ARecipeVersionID(-1))
      vid = recipe.getLatestVersion().id;

    //Save the instructions, and get their dependencies
    QVector<std::pair<ARecipeID, ARecipeVersionID>> dependencies;
    QJsonArray recipe_instructions = instructionsToJson(recipe, dependencies);

    //Save the version
    QJsonArray recipe_history;
    //If we have already saved another version of this recipe, append to it
    if(saved_versions.end() != std::find_if(saved_versions.begin(), saved_versions.end(),
                 [rid](const std::pair<ARecipeID, ARecipeVersionID> &rid_vid) { return rid == rid_vid.first; })) {
      for(int i = 0; i < recipes.size(); i++) {
        if(recipes[i].toObject()["id"].toInt() == (int)rid.val()) {
          recipe_history = recipes[i].toObject()["history"].toArray();
          break;
        }
      }
    }
    const ARecipeVersion &version = recipe.getVersion(vid);
    recipe_history.append(versionToJson(version));

    //Wrap up the recipe
    QJsonObject json_recipe;
    json_recipe["name"] = QString::fromStdString(recipe.name());
    json_recipe["instructions"] = recipe_instructions;
    json_recipe["history"] = recipe_history;
    json_recipe["id"] = (int)rid.val();

    //Store it, and signal that
    recipes.append(json_recipe);
    saved_versions.push_back(std::make_pair(rid, vid));

    if(recursive) {
      for(const auto &dep : dependencies)
        saveVersion(dep.first, dep.second);
    }
  }

  QJsonObject toJson(ARecipeID current_rid) {
    QJsonObject state;
    state["current rid"] = (int)current_rid.val();
    state["recipes"] = recipes;
    return state;
  }
};


ARecipeID ARepository::recipeFromJson(const QJsonObject &state)
{
  std::vector<AInstructionID> new_instructions;
  for(const QJsonValue &v : state["instructions"].toArray()) {
    AInstructionID new_iid = instructionFromJson(v.toObject());
    if(new_iid == AInstructionID(-1))
      return invalid_recipe_id;
    new_instructions.push_back(new_iid);
  }
  if(new_instructions.empty()) {
    AInstruction *new_instr = new Instructions::Inherit(next_rid, ARecipeVersionID(-1), "");
    AInstructionID new_iid = addInstruction(std::unique_ptr<AInstruction>(new_instr));
    new_instructions.push_back(new_iid);
  }

  std::vector<ARecipeVersion> recipe_history;
  for(const QJsonValue &v : state["history"].toArray())
    recipe_history.push_back(versionFromJson(v.toObject()));


  ARecipeID new_recipe_id(state["id"].toInt());
  if(recipes.end() != std::find_if(recipes.begin(), recipes.end(),
                                   [new_recipe_id](const std::pair<ARecipeID, ARecipe> &key_val) { return new_recipe_id == key_val.first; })) {
    qDebug()<<"Trying to load already existing recipe ID. Changing ID, but this may have consequences in dependencies!";
    new_recipe_id = next_rid;
  }

  std::string name = state["name"].toString().toLocal8Bit().data();
  ARecipe new_recipe(new_instructions, recipe_history, name);
  recipes.emplace(new_recipe_id, std::move(new_recipe));
  if(next_rid < new_recipe_id) {
    next_rid = new_recipe_id;
    ++next_rid;
  } else if(next_rid == new_recipe_id)
    ++next_rid;


  emit recipeChanged(new_recipe_id);
  return new_recipe_id;
}

AInstructionID ARepository::instructionFromJson(const QJsonObject &state)
{
  AInstruction *instruction = AInstruction::fromJson(state);
  AInstructionID iid;
  if(!instruction) return AInstructionID(-1);
  if(!findInstruction(*instruction, &iid))
    iid = addInstruction(std::unique_ptr<AInstruction>(instruction));
  return iid;
}

ARecipeVersion ARepository::versionFromJson(const QJsonObject &state)
{
  ARecipeVersion version;
  version.name = state["name"].toString().toLocal8Bit().data();
  version.id = ARecipeVersionID(state["id"].toInt());
  version.sensors = ASensorGroup(state["sensors"]);
  version.creation_time = state["creation time"].toString().toLocal8Bit().data();
  version.comment = state["comment"].toString().toLocal8Bit().data();
  if(next_vid < version.id)
    next_vid = version.id;
  ++next_vid;
  return version;
}

void ARepository::stopUpdate()
{
  f_continue_update = false;
}

void ARepository::setNextUpdateConfig(const QJsonObject &config)
{
  //qDebug()<<"Updated:" << config;
  next_update_config = config;
  emit nextUpdateConfigChanged(config);
}

bool ARepository::mergeRepository(const ARepository &other)
{
  ARecipeID min_rid = other.next_rid;
  ARecipeVersionID min_vid = other.next_vid;

  //Determine other min IDs, and map new instructions
  AInstructionID next_iid_tmp = next_iid;
  std::map<AInstructionID, AInstructionID> other_to_this_iid;
  for(auto other_recipes_key_val : other.recipes) {
    ARecipeID rid = other_recipes_key_val.first;
    if(rid < min_rid) min_rid = rid;

    const ARecipe &recipe = other_recipes_key_val.second;
    for(const ARecipeVersion &version : recipe.history())
      if(version.id < min_vid) min_vid = version.id;

    for(AInstructionID other_iid : recipe.getInstructions()) {
      auto iid_map_key_val = other_to_this_iid.find(other_iid);
      if(iid_map_key_val == other_to_this_iid.end()) {
        if(!other.hasInstruction(other_iid)) {
          qDebug()<<"ERROR: Unable to retrieve used instruction from other repository.";
          return false;
        }
        const AInstruction &other_instruction = other.getInstruction(other_iid);
        AInstructionID this_iid;
        if(!this->findInstruction(other_instruction, &this_iid))
          this_iid = next_iid_tmp++;
        other_to_this_iid[other_iid] = this_iid;
      }
    }
  }

  //Calculate ID diffs and map next IDs
  ARecipeID rid_shift(0);
  if(min_rid < this->next_rid) rid_shift = this->next_rid - min_rid;
  this->next_rid = other.next_rid+rid_shift;
  ARecipeVersionID vid_shift(0);
  if(min_vid < this->next_vid) vid_shift = this->next_vid - min_vid;
  this->next_vid = other.next_vid+vid_shift;

  std::map<ARecipeID, ARecipeID> other_to_this_rid;
  std::map<ARecipeVersionID, ARecipeVersionID> other_to_this_vid;
  for(auto other_recipes_key_val : other.recipes) {
    other_to_this_rid[other_recipes_key_val.first] = other_recipes_key_val.first + rid_shift;
    for(auto &version : other_recipes_key_val.second.history()) {
      other_to_this_vid[version.id] = version.id + vid_shift;
    }
  }

  //Copy instructions, remapping their IDs
  for(auto other_recipes_key_val : other.recipes) {
    const ARecipe &recipe = other_recipes_key_val.second;
    for(AInstructionID other_iid : recipe.getInstructions()) {
      const AInstruction &other_instruction = other.getInstruction(other_iid);
      AInstructionID this_iid;
      if(!this->findInstruction(other_instruction, &this_iid)) {
        AInstruction *this_instruction = other_instruction.clone();
        if(this_instruction == nullptr) {
          qDebug()<<"ERROR: Failed to clone other_instruction.";
          return false;
        }
        this_instruction->remapInstructionIDs(other_to_this_iid);
        this_instruction->remapRecipeIDs(other_to_this_rid);
        this_instruction->remapVersionIDs(other_to_this_vid);
        this_iid = addInstruction(std::unique_ptr<AInstruction>(this_instruction));
      }
    }
  }

  //Copy recipes and version with new IDs
  for(auto key_val : other.recipes) {
    ARecipeID rid = key_val.first+rid_shift;
    ARecipe &recipe = recipes[rid];
    recipe = key_val.second;
    recipe.remapInstructionIDs(other_to_this_iid);
    for(ARecipeVersion &version : recipe.history())
      version.id = version.id + vid_shift;
    emit recipeChanged(rid);
  }

  if(other.current_rid != invalid_recipe_id && other_to_this_rid.find(other.current_rid) != other_to_this_rid.end())
    setCurrentRecipe(other_to_this_rid[other.current_rid]);
  return true;
}

QJsonObject ARepository::toJson()
{
  ARepositorySave save(this);
  for(auto &key_val : this->recipes)
    save.saveRecipe(key_val.first, false);
  return save.toJson(getCurrentRecipeID());
}

QJsonObject ARepository::toJson(ARecipeID rid)
{
  if(!hasRecipe(rid)) return QJsonObject();
  ARepositorySave save(this);
  save.saveRecipe(rid);
  return save.toJson(rid);
}

QJsonObject ARepository::toJson(ARecipeID rid, ARecipeVersionID vid)
{
  if(!hasVersion(rid, vid)) return QJsonObject();
  ARepositorySave save(this);
  save.saveVersion(rid, vid);
  return save.toJson(rid);
}

ARepository::ARepository(int numPMs)
{
  sensor_count = numPMs;
  next_iid = AInstructionID(0);
  clear(sensor_count);
}

ARepository::ARepository(const QJsonObject &state, int numPMs)
{
  sensor_count = numPMs;
  next_iid = AInstructionID(0);
  clear(sensor_count);

  ARecipeID last_recipe;

  //Load whatever there is in JSON
  if(state["recipes"].isArray()) {
    //We have a whole repository. Load each recipe.
    for(const QJsonValue &v : state["recipes"].toArray())
      recipeFromJson(v.toObject());
    last_recipe = ARecipeID(state["current rid"].toInt());
  }
  else if(state["history"].isArray() || state["instructions"].isArray())
    //We have a recipe. Load it.
    last_recipe = recipeFromJson(state);
  else if(state["sensors"].isArray() && state["id"].isDouble()) {
    //We have a version. Wrap it in an instructionless recipe.
    QJsonArray hist; hist.append(state);
    QJsonObject new_state;
    new_state["history"] = hist;
    new_state["name"] = "R"+QString::number(next_rid.val())+" - Loaded Version";
    last_recipe = recipeFromJson(new_state);
  }
  setCurrentRecipe(last_recipe);
}

void ARepository::clear(int numPMs)
{
  unsetCurrentLrfs();
  unsetSecondaryLrfs();
  sensor_count = numPMs;
  recipes.clear();
  next_rid = ARecipeID(0);
  next_vid = ARecipeVersionID(0);
  emit repositoryCleared();
}

ARecipeID ARepository::addRecipe(const std::string &name, const std::vector<AInstructionID> &instructions)
{
  ARecipe recipe(name);
  for(auto iid : instructions)
    recipe.addInstruction(iid);
  ARecipeID new_id = recipes.emplace(next_rid++, std::move(recipe)).first->first;
  if(current_rid == invalid_recipe_id)
    setCurrentRecipe(new_id);
  return new_id;
}

void ARepository::remove(ARecipeID rid)
{
  if(!hasRecipe(rid))
    return;
  if(recipes.size() > 1) {
    if(rid == current_rid)
      unsetCurrentLrfs();
    if(rid == secondary_rid)
      unsetSecondaryLrfs();
    recipes.erase(rid);
    emit recipeChanged(rid);
  }
  else
    clear(sensor_count);
}

bool ARepository::anyDependsOn(ARecipeID rid) const
{
  for(auto &key_val : instructions) {
    const AInstruction *instruction = key_val.second.get();
    for(auto dep : instruction->getDependencies())
      if(dep.first == rid)
        return true;
  }
  return false;
}

bool ARepository::anyNonCircularDependsOn(ARecipeID rid) const
{
  for(auto &key_val : instructions) {
    AInstructionID iid = key_val.first;
    const AInstruction *instruction = key_val.second.get();
    for(auto instruction_dependency : instruction->getDependencies()) {
      if(instruction_dependency.first == rid) { //If iid depends on rid
        //Check if other recipes have iid
        for(auto other_recipe : recipes) {
          if(other_recipe.first != rid && other_recipe.second.hasInstruction(iid))
            return true;
        }
        break; //Nothing more to be checked with this iid
      }
    }
  }
  return false;
}

const std::string &ARepository::getName(ARecipeID rid) const
{
  return getRecipe(rid).name();
}

bool ARepository::setName(ARecipeID rid, const std::string &name)
{
  if(rid == invalid_recipe_id) return false;
  auto key_val = recipes.find(rid);
  if(key_val != recipes.end()) {
    key_val->second.setName(name);
    emit recipeChanged(rid);
    return true;
  }
  return false;
}

bool ARepository::updateRecipe(AInstructionInput &data, ARecipeID rid)
{
  if(rid == invalid_recipe_id)
    return false;
  auto it = recipes.find(rid);
  if(it == recipes.end())
    return false;
  ARecipe &recipe = it->second;
  int instruction_count = recipe.getInstructionCount();
  if(instruction_count == 0)
    return false;

  f_continue_update = true;
  ASensorGroup sensors;
  emit reportProgress(0);
  for(int i = 0; i < instruction_count; i++) {
    if(!f_continue_update)
      return false;

    data.setProgressReporter([=](float progress) {
      if(progress < 0) progress = 0;
      else if(progress > 1) progress = 1;
      emit reportProgress((progress+i)*100.0f/instruction_count);
      qApp->processEvents();
      return f_continue_update;
    });

    AInstructionID iid = recipe.getInstruction(i);
    if(!hasInstruction(iid)) {
      qDebug()<<"ARepository::updateRecipe(): Recipe has instruction not present in repository. Should never happen!";
      continue;
    }
    const AInstruction &instruction = getInstruction(iid);
    if(!instruction.apply(sensors, data)) {
      qDebug()<<"Failed to apply instruction!";
      return false;
    }
    emit reportProgress((i+1)*100.0f/instruction_count);
    qApp->processEvents();
  }

  recipe.addVersion(next_vid, std::move(sensors));
  emit recipeChanged(rid);
  next_vid++;
  return true;
}

std::vector<ARecipeID> ARepository::getRecipeList() const
{
  return getMapKeys(recipes);
}

bool ARepository::findRecipe(const std::vector<AInstructionID> &instructions, ARecipeID *rid) const
{
  for(auto &key_val : recipes)
    if(key_val.second.hasSameInstructions(instructions)) {
      if(rid != nullptr) *rid = key_val.first;
      return true;
    }
  return false;
}

bool ARepository::findRecipe(const std::string &name, ARecipeID *rid) const
{
  for(auto &key_val : recipes)
    if(key_val.second.name() == name) {
      if(rid) *rid = key_val.first;
      return true;
    }
  return false;
}

bool ARepository::hasVersion(ARecipeID rid, ARecipeVersionID vid) const
{
  if(hasRecipe(rid))
    return getRecipe(rid).hasVersion(vid);
  else return false;
}

ARecipeVersion &ARepository::getVersionRw(ARecipeID rid, ARecipeVersionID vid)
{
  return getRecipe(rid).getVersion(vid);
}

const ARecipeVersion &ARepository::getVersion(ARecipeID rid, ARecipeVersionID vid) const
{
  return getRecipe(rid).getVersion(vid);
}

const std::vector<ARecipe::Version> &ARepository::getHistory(ARecipeID rid) const
{
  return getRecipe(rid).history();
}

bool ARepository::setVersionComment(ARecipeID rid, ARecipeVersionID vid, const std::string &comment)
{
  if(hasVersion(rid, vid)) {
    getVersionRw(rid, vid).comment = comment;
    return true;
  }
  return false;
}

bool ARepository::renameVersion(ARecipeID rid, ARecipeVersionID vid, const std::string &new_name)
{
  if(!hasVersion(rid, vid))
    return false;
  getVersionRw(rid, vid).name = new_name;
  emit recipeChanged(rid);
  return true;
}

bool ARepository::removeVersion(ARecipeID rid, ARecipeVersionID vid)
{
  if(!hasRecipe(rid))
    return false;
  ARecipe &recipe = recipes.at(rid);
  bool ok = recipe.removeVersion(vid);
  if(ok) {
    if(vid == current_version.id)
      unsetCurrentLrfs();
    if(vid == secondary_version.id)
      unsetSecondaryLrfs();
    emit recipeChanged(rid);
  }
  return ok;
}

ARecipeID ARepository::editSensorLayer(ARecipeID rid, ARecipeVersionID vid, int isensor, unsigned layer, double *coeff, std::shared_ptr<const ALrf> lrf)
{
  //Check if layer exists
  if(!hasVersion(rid, vid)) return invalid_recipe_id;
  const ARecipe &recipe = getRecipe(rid);
  const ARecipeVersion &version = recipe.getVersion(vid);
  const ASensor *sensor = version.sensors.getSensor(isensor);
  if(!sensor) return invalid_recipe_id;
  if(sensor->deck.size() <= layer) return invalid_recipe_id;

  //Check if lrf is unchanged
  bool is_different_coeff = false;
  if(coeff != nullptr)
    is_different_coeff = sensor->deck[layer].coeff != *coeff;
  bool is_different_lrf = false;
  if(lrf != nullptr) {
    QJsonObject current_state = ALrfTypeManager::instance().lrfToJson(sensor->deck[layer].lrf.get());
    QJsonObject new_state = ALrfTypeManager::instance().lrfToJson(lrf.get());
    if(current_state != new_state)
      is_different_lrf = true;
  }
  if(!is_different_coeff && !is_different_lrf)
    return rid;

  //Check if recipe is editable
  bool is_editable = false;
  if(recipe.history().size() == 1 && recipe.getInstructionCount() == 1) {
    AInstructionID iid = recipe.getInstruction(0);
    if(!hasInstruction(iid)) {
      qDebug()<<"ARepository::editSensorLayer(): Recipe has instruction not present in repository. Should never happen!";
      return invalid_recipe_id;
    }
    auto *instr = dynamic_cast<const Instructions::Inherit *>(&getInstruction(iid));
    if(instr && instr->getDependencies()[0].first == rid)
      is_editable = true;
  }

  //Create instruction of self-reference
  ARecipeID new_rid = is_editable ? rid : next_rid;
  if(!is_editable) ++next_rid;
  auto new_instruction = std::unique_ptr<AInstruction>(new Instructions::Inherit(new_rid, ARecipeVersionID(-1), ""));
  std::vector<AInstructionID> new_instructions(1, addInstruction(std::move(new_instruction)));

  //Create new version
  ARecipeVersion new_version = version;
  if(!is_editable) new_version.id = next_vid++;
  ASensor::Parcel *new_parcel = &new_version.sensors.getSensor(isensor)->deck[layer];
  if(coeff != nullptr)
    new_parcel->coeff = *coeff;
  if(lrf != nullptr)
    new_parcel->lrf = lrf;
  new_version.setCreationTime(std::time(nullptr));
  std::vector<ARecipeVersion> new_history(1, std::move(new_version));

  ARecipe new_recipe(new_instructions, new_history, recipe.name()+" (edited)");
  recipes[new_rid] = std::move(new_recipe);
  emit recipeChanged(new_rid);
  emit currentLrfsChanged(new_rid, new_version.id);
  emit currentLrfsChangedReadyStatus(isAllLRFsDefined());
  return new_rid;
}

bool ARepository::setCurrentRecipe(ARecipeID rid, ARecipeVersionID vid)
{
  if(!hasVersion(rid, vid))
    return false;

  current_rid = rid;
  current_vid = vid;
  current_version = getVersion(rid, vid);

  if(rid == secondary_rid && current_version.id == secondary_version.id)
    unsetSecondaryLrfs();

  emit currentLrfsChanged(current_rid, current_vid);
  emit currentLrfsChangedReadyStatus(isAllLRFsDefined());
  return true;
}

void ARepository::unsetCurrentLrfs()
{
  current_rid = invalid_recipe_id;
  current_vid = ARecipeVersionID(-1);
  current_version = ARecipeVersion();
  emit currentLrfsChanged(current_rid, current_vid);
  emit currentLrfsChangedReadyStatus(false);
}

bool ARepository::setSecondaryRecipe(ARecipeID rid, ARecipeVersionID vid)
{
  if(!hasVersion(rid, vid))
    return false;

  const ARecipeVersion &version = getVersion(rid, vid);

  if(rid == current_rid && current_version.id == version.id)
    return false;

  secondary_rid = rid;
  secondary_vid = vid;
  secondary_version = version;
  emit secondaryLrfsChanged(secondary_rid, secondary_vid);
  return true;
}

void ARepository::unsetSecondaryLrfs()
{
  secondary_rid = invalid_recipe_id;
  secondary_vid = ARecipeVersionID(-1);
  secondary_version = ARecipeVersion();
  emit secondaryLrfsChanged(secondary_rid, secondary_vid);
}

bool ARepository::isAllLRFsDefined() const
{
  if(sensor_count == 0) return false;
  const ASensorGroup &current = getCurrentLrfs();
  return (int)current.size() == sensor_count;
}


const AInstruction &ARepository::getInstruction(AInstructionID iid) const
{
  return *instructions.find(iid)->second.get();
}

AInstructionID ARepository::addInstruction(std::unique_ptr<const AInstruction> instruction)
{
  AInstructionID iid;
  if(!this->findInstruction(*instruction.get(), &iid))
    iid = instructions.emplace(next_iid++, std::move(instruction)).first->first;
  return iid;
}

bool ARepository::remove(AInstructionID iid)
{
  if(anyDependsOn(iid))
    return false;
  return instructions.erase(iid) > 0;
}

bool ARepository::anyDependsOn(AInstructionID iid)
{
  for(auto &key_val : recipes) {
    if(key_val.second.hasInstruction(iid))
      return true;
  }
  return false;
}

void ARepository::removeUnusedInstructions()
{
  auto end = instructions.end();
  for(auto it = instructions.begin(); it != end;) {
    if(anyDependsOn(it->first)) ++it;
    else it = instructions.erase(it);
  }
}

std::vector<AInstructionID> ARepository::getInstructionList() const
{
  return getMapKeys(instructions);
}

const std::vector<AInstructionID> &ARepository::getInstructionList(ARecipeID rid) const
{
  return getRecipe(rid).getInstructions();
}

bool ARepository::findInstruction(const AInstruction &instruction, AInstructionID *iid) const
{
  for(auto &key_val : instructions)
    if(*key_val.second.get() == instruction) {
      if(iid) *iid = key_val.first;
      return true;
    }
  return false;
}

ARecipe &ARepository::getRecipe(ARecipeID rid)
{
  return recipes.find(rid)->second;
}

const ARecipe &ARepository::getRecipe(ARecipeID rid) const
{
  return recipes.find(rid)->second;
}

} //namespace LRF
