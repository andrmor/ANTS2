#ifndef LRF_ADETECTOR_H
#define LRF_ADETECTOR_H

#include <vector>
#include <memory>

#include "idclasses.h"
#include "asensor.h"

namespace LRF {

class AInstructionInput;
class ARepository;

class ADetector
{
public:
  typedef std::pair<std::string, ASensorGroup> type_version;
private:
  std::vector<AInstructionID> instructions;
  std::vector<type_version> versions;
public:
  std::string name_;
  const std::string &name() const { return name_; }
  ADetector(const std::string &name = "Detector") : name_(name) { }

  bool isEmpty() const { return versions.empty(); }

  bool addInstruction(AInstructionID instruction);
  bool removeInstruction(AInstructionID instruction);
  bool hasInstruction(AInstructionID instruction);
  const std::vector<AInstructionID> &getInstructions() const { return instructions; }

  const type_version *getVersion(const std::string &version) const;
  bool removeVersion(const std::string &version);
  const std::vector<type_version> &history() const { return versions; }
  std::vector<type_version> &history() { return versions; }

  void clearHistory();
  const type_version &getLatestVersion() const { return versions.back(); }
#if 0
  const ASensorGroup *getCurrentVersion() const {
    size_t i = current.val();
    return i < versions.size() ? &versions[i].second : nullptr;
  }
  bool setCurrentVersion(const std::string &version);
#endif

  bool update(const ARepository *repo, const AInstructionInput &data);
};

} //namespace LRF

#endif // LRF_ADETECTOR_H
