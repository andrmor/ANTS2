#ifndef LRF_AREPOSITORY_H
#define LRF_AREPOSITORY_H

#include <map>
#include <memory>

#include <QObject>

#include "idclasses.h"
#include "ainstruction.h"
#include "arecipe.h"

namespace LRF {

class ARepository : public QObject
{
  Q_OBJECT
  friend class ARepositorySave;

  QJsonObject next_update_config;
  bool f_continue_update;
  int sensor_count;

  std::map<AInstructionID, std::unique_ptr<const AInstruction>> instructions;
  AInstructionID next_iid;

  std::map<ARecipeID, ARecipe> recipes;
  ARecipeID next_rid;
  ARecipeID current_rid;
  ARecipeVersionID current_vid;
  ARecipeID secondary_rid;
  ARecipeVersionID secondary_vid;

  ARecipeVersion current_version;
  ARecipeVersion secondary_version;
  ARecipeVersionID next_vid;

  //If any of these are not in the json loader/constructor something might be wrong.
  ARecipeID recipeFromJson(const QJsonObject &state);
  AInstructionID instructionFromJson(const QJsonObject &state);
  ARecipeVersion versionFromJson(const QJsonObject &state);

signals:
  void reportProgress(int progress);
  void repositoryCleared();
  void recipeChanged(ARecipeID rid);
  void currentLrfsChangedReadyStatus(bool allLrfsDefined);
  void currentLrfsChanged(ARecipeID new_rid, ARecipeVersionID new_vid);
  void secondaryLrfsChanged(ARecipeID new_rid, ARecipeVersionID new_vid);
  void nextUpdateConfigChanged(const QJsonObject &config);

public slots:
  void stopUpdate();

public:
  QJsonObject getNextUpdateConfig() { return next_update_config; }
  void setNextUpdateConfig(const QJsonObject &config);

  ///
  /// \brief Adds the instructions, recipes, and the history of other into this
  ///  instance.
  ///
  bool mergeRepository(const ARepository &other);

  QJsonObject toJson();
  QJsonObject toJson(ARecipeID rid);
  QJsonObject toJson(ARecipeID rid, ARecipeVersionID vid);

  ARepository(int numPMs = 0);
  ARepository(const QJsonObject &state, int numPMs = 0);

  ///
  /// \brief Compatibility: Removes all recipes and instructions, and sets the
  ///  number of sensors to numPMs.
  ///
  void clear(int numPMs);

  //Curent Recipe
  const ASensorGroup &getCurrentLrfs() const { return current_version.sensors; }
  ARecipeID getCurrentRecipeID() const { return current_rid; }
  ARecipeVersionID getCurrentRecipeVersionID() const { return current_vid; }
  bool setCurrentRecipe(ARecipeID rid, ARecipeVersionID vid = ARecipeVersionID(-1));
  void unsetCurrentLrfs();
  bool isCurrentRecipeSet() const { return current_rid != invalid_recipe_id; }

  //Secondary Recipe
  const ASensorGroup &getSecondaryLrfs() const { return secondary_version.sensors; }
  ARecipeID getSecondaryRecipeID() const { return secondary_rid; }
  ARecipeVersionID getSecondaryRecipeVersionID() const { return secondary_vid; }
  bool setSecondaryRecipe(ARecipeID rid, ARecipeVersionID vid = ARecipeVersionID(-1));
  void unsetSecondaryLrfs();
  bool isSecondaryRecipeSet() const { return secondary_rid != invalid_recipe_id; }

  //Compatibility
  bool isAllLRFsDefined() const;
  double getLRF(int pmt, const APoint &pos) { return getCurrentLrfs()[pmt].eval(pos); }
  double getLRFErr(int pmt, const APoint &pos) { return getCurrentLrfs()[pmt].sigma(pos); }


  //Recipes
  ARecipeID getNextRecipeId() const { return next_rid; }
  ARecipeID addRecipe(const std::string &name, const std::vector<AInstructionID> &instructions);
  void remove(ARecipeID rid);
  bool hasRecipe(ARecipeID rid) const { return recipes.find(rid) != recipes.end(); }
  bool anyDependsOn(ARecipeID rid) const;
  bool anyNonCircularDependsOn(ARecipeID rid) const;

  const std::string &getName(ARecipeID rid) const;
  bool setName(ARecipeID rid, const std::string &name);

  ///
  /// \brief Updates (make lrfs) the recipe with ID rid.
  ///
  bool updateRecipe(AInstructionInput &data, ARecipeID rid);
  bool updateRecipe(AInstructionInput &data) { return updateRecipe(data, current_rid); }

  ///
  /// \brief Gets a container with the list of all the IDs of all ARecipes.
  ///
  std::vector<ARecipeID> getRecipeList() const;

  ///
  /// \brief Finds the recipe which contains exactly the set of given instructions.
  /// \return True if such recipe is found. False otherwise. If a recipe is
  ///  found, and rid != nullptr, it is set with the ID of the recipe.
  ///
  bool findRecipe(const std::vector<AInstructionID> &instructions, ARecipeID *rid = nullptr) const;
  bool findRecipe(const std::string &name, ARecipeID *rid = nullptr) const;


  //Versions of Recipes
  bool hasVersion(ARecipeID rid, ARecipeVersionID vid) const;
  const ARecipeVersion &getVersion(ARecipeID rid, ARecipeVersionID vid) const;
  const std::vector<ARecipe::Version> &getHistory(ARecipeID rid) const;
  bool setVersionComment(ARecipeID rid, ARecipeVersionID vid, const std::string &comment);
  bool renameVersion(ARecipeID rid, ARecipeVersionID vid, const std::string &new_name);
  bool removeVersion(ARecipeID rid, ARecipeVersionID vid);


  ///
  /// \brief Changes the lrf of a sensor layer. If necessary, a new recipe is created
  /// \return If layer doesn't exist, invalid_recipe_id is returned. If the
  ///  recipe is editable or the lrf is unchanged it returns rid, otherwise it
  ///  returns the ID of a new (edited) recipe.
  ///
  ARecipeID editSensorLayer(ARecipeID rid, ARecipeVersionID vid, int isensor,
                            unsigned layer, double *coeff = nullptr, std::shared_ptr<const ALrf> lrf = nullptr);

  //Instructions
  bool hasInstruction(AInstructionID iid) const { return instructions.find(iid) != instructions.end(); }
  const AInstruction &getInstruction(AInstructionID iid) const;
  //The unique_ptr is to enforce that ARepository owns the instruction now.
  AInstructionID addInstruction(std::unique_ptr<const AInstruction> instruction);
  bool remove(AInstructionID iid);
  bool anyDependsOn(AInstructionID iid);
  void removeUnusedInstructions();
  std::vector<AInstructionID> getInstructionList() const;
  const std::vector<AInstructionID> &getInstructionList(ARecipeID rid) const;

  bool findInstruction(const AInstruction &instruction, AInstructionID *iid = nullptr) const;

private:
  ARecipe &getRecipe(ARecipeID rid);
  const ARecipe &getRecipe(ARecipeID rid) const;
  ARecipeVersion &getVersionRw(ARecipeID rid, ARecipeVersionID vid);
};

}

#endif // LRF_AREPOSITORY_H
