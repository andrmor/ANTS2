#ifndef LRF_ARECIPE_H
#define LRF_ARECIPE_H

#include <vector>
#include <map>
#include <memory>
#include <ctime>

#include "idclasses.h"
#include "asensor.h"

namespace LRF {

class AInstructionInput;
class ARepository;

class ARecipeVersion {
public:
  ARecipeVersionID id;
  std::string name;
  ASensorGroup sensors;
  std::string comment;
  std::string creation_time;

  ARecipeVersion() { setCreationTime(std::time(nullptr)); }

  ARecipeVersion(ARecipeVersionID id, std::string name, ASensorGroup sensors)
    : id(id), name(std::move(name)), sensors(std::move(sensors))
  { setCreationTime(std::time(nullptr)); }

  void setCreationTime(const time_t &time) { creation_time = std::asctime(std::gmtime(&time)); }
};

class ARecipe
{
  std::vector<AInstructionID> instructions;
  std::vector<ARecipeVersion> versions;
  std::string name_;

public:
  typedef ARecipeVersion Version;

  ARecipe(const std::string &name = "") : name_(name) {}
  ARecipe(std::vector<AInstructionID> instructions, std::vector<ARecipeVersion> versions, const std::string &name = "")
    : instructions(instructions), versions(versions), name_(name) {}

  const std::string &name() const { return name_; }
  void setName(const std::string & name) { this->name_ = name; }

  bool isEmpty() const { return versions.empty(); }

  bool addInstruction(AInstructionID instruction);
  bool hasInstruction(AInstructionID instruction) const;
  size_t getInstructionCount() const { return instructions.size(); }
  AInstructionID getInstruction(size_t i) const { return instructions[i]; }
  const std::vector<AInstructionID> &getInstructions() const { return instructions; }
  bool hasSameInstructions(const std::vector<AInstructionID> &instructions) const
  { return instructions == this->instructions; }

  Version &getVersion(ARecipeVersionID vid);
  const Version &getVersion(ARecipeVersionID vid) const;
  bool hasVersion(ARecipeVersionID vid) const;
  void addVersion(ARecipeVersionID vid, ASensorGroup sensors);
  bool removeVersion(ARecipeVersionID vid);
  const std::vector<Version> &history() const { return versions; }
  std::vector<Version> &history() { return versions; }

  const Version &getLatestVersion() const { return versions.back(); }

  void remapInstructionIDs(const std::map<AInstructionID, AInstructionID> &map);

  bool operator==(const ARecipe &other) const { return instructions == other.instructions; }
  bool operator!=(const ARecipe &other) const { return instructions != other.instructions; }
};

} //namespace LRF

#endif // LRF_ARECIPE_H
