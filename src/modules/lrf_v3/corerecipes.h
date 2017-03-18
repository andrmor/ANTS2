#ifndef LRFCORERECIPES_H
#define LRFCORERECIPES_H

#include <vector>

#include "arecipe.h"
#include "atransform.h"

namespace LRF {

class ALrfTypeManager;

namespace CoreRecipes {

class FitLayer : public ARecipe
{
  QJsonObject settings, lrf_settings;
  const ALrfType *lrf_type;
  //std::vector<int> pm_selection;
  int group_type, stack_op;
  bool adjust_gains;
public:
  FitLayer(const QJsonObject &settings);
  bool apply(ASensorGroup &group, const ARecipeInput &input) const override;
  std::vector<ADetectorID> detectorDepends() const override;
  QJsonObject toJson() const override;
  const std::string &type() const override { return "FitLayer"; }
};

} } //namespace LRF::Recipes

#endif // LRFCORERECIPES_H
