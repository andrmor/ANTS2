#ifndef ALRFINSTRUCTION_H
#define ALRFINSTRUCTION_H

#include <map>
#include <vector>

#include <QVector>
#include <QJsonObject>

#include "idclasses.h"

namespace LRF {

class ARepository;
class ASensorGroup;
class AInstructionInput;

class AInstruction {
  std::string name_;
  static AInstruction *createInstruction(const std::string &type_name, const std::string &name, const QJsonObject &settings);
protected:
  ///
  /// \brief Retrieves the settings of the subclass
  ///
  virtual QJsonObject toJsonImpl() const = 0;
public:
  virtual ~AInstruction() {}

  const std::string &name() const { return name_; }
  void setName(const std::string &name) { this->name_ = name; }

  ///
  /// \brief Applies the instruction to group
  /// \param group The ASensorGroup to be transformed
  /// \return true if the operation succeded, false otherwise
  ///
  virtual bool apply(ASensorGroup &group, const AInstructionInput &input) const = 0;

  ///
  /// \brief Returns the Recipe/version pairs that this instruction depends on
  ///
  virtual std::vector<std::pair<ARecipeID,ARecipeVersionID>> getDependencies() const
  { return std::vector<std::pair<ARecipeID,ARecipeVersionID>>(0); }

  //Check class Instructions::Inherit for info on these
  virtual void remapInstructionIDs(const std::map<AInstructionID, AInstructionID> &/*map*/) {}
  virtual void remapRecipeIDs(const std::map<ARecipeID, ARecipeID> &/*map*/) {}
  virtual void remapVersionIDs(const std::map<ARecipeVersionID, ARecipeVersionID> &/*map*/) {}

  ///
  /// \brief Returns the type name of the instruction
  /// \details Don't forget to match the names with the ones in ainstruction.cpp
  /// alrfwindowwidgets.cpp
  ///
  virtual std::string typeName() const = 0;

  ///
  /// \brief Returns true if other is equal to this object, including type
  ///
  virtual bool operator==(const AInstruction &other) const;

  AInstruction *clone() const;

  ///
  /// \brief Exports the information required to recreate the object
  /// \return A JSON value with the information
  ///
  QJsonObject toJson() const;

  static AInstruction *fromJson(const QJsonObject &json);
};

class AFitLayerSensorGroup;

namespace Instructions {

class FitLayer : public AInstruction
{
  QJsonObject settings;
  QJsonObject lrf_settings;
  QString sensor_list_txt;
  const ALrfType *lrf_type;
  int reconstruction_group;
  int group_type, stack_op;
  bool group_enabled, adjust_gains;
protected:
  QJsonObject toJsonImpl() const override;
public:
  FitLayer(const QJsonObject &settings);
  std::vector<AFitLayerSensorGroup> getSymmetryGroups(const AInstructionInput &input) const;
  bool apply(ASensorGroup &group, const AInstructionInput &input) const override;
  std::string typeName() const override { return "Fit Layer"; }
};

class Inherit : public AInstruction
{
  ARecipeID rid;
  ARecipeVersionID vid;
  QString sensors;
protected:
  QJsonObject toJsonImpl() const override;
public:
  Inherit(ARecipeID rid, ARecipeVersionID vid, const QString &sensors);
  Inherit(const QJsonObject &settings);
  bool apply(ASensorGroup &group, const AInstructionInput &input) const override;
  std::vector<std::pair<ARecipeID,ARecipeVersionID>> getDependencies() const override;
  void remapRecipeIDs(const std::map<ARecipeID, ARecipeID> &map) override;
  void remapVersionIDs(const std::map<ARecipeVersionID, ARecipeVersionID> &map) override;
  std::string typeName() const override { return "Inherit"; }
};

} //namespace CoreInstructions

} //namespace LRF

#endif // ALRFINSTRUCTION_H
