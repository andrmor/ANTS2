#ifndef LRFIDCLASSES_H
#define LRFIDCLASSES_H

#include "aid.h"

namespace LRF {

class AInstruction;
typedef AId<AInstruction> AInstructionID;

class ARecipe;
typedef AId<ARecipe> ARecipeID;
extern const ARecipeID invalid_recipe_id;

class ALrfType;
//There is code that assumes this is unsigned!
typedef AId<ALrfType, unsigned int> ALrfTypeID;

class ARecipeVersion;
typedef AId<ARecipeVersion> ARecipeVersionID;

}

#endif // LRFIDCLASSES_H
