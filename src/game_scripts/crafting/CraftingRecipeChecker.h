#pragma once

#include "CraftingTypes.h"

#include <vector>
#include <algorithm>

namespace Crafting{
    class CraftingRecipeChecker{
    public:
        static bool IsVodkaRecipe(
            const std::vector<IngredientType>& ingredients){
            bool hasWater = false;
            bool hasSugar = false;

            for (auto ingredient : ingredients){
                if (ingredient == IngredientType::Water){
                  hasWater = true;
                }

                if (ingredient == IngredientType::Sugar){
                  hasSugar = true;
                }
            }

            return hasWater && hasSugar;
        }
    };
}