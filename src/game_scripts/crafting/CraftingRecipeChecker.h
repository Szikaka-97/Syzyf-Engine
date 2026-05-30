#pragma once

#include "CraftingTypes.h"

#include <string>
#include <vector>

namespace Crafting{
    struct CraftingRecipeCheckResult{
        bool valid = false;
        std::string recipeName = "Invalid";
        std::string reason = "Recipe is invalid.";
    };

    class CraftingRecipeChecker{
    public:
        static CraftingRecipeCheckResult Check(const std::vector<IngredientData>& ingredients){
            CraftingRecipeCheckResult result;

            if (ingredients.empty()){
                result.reason = "No ingredients inserted.";
                return result;
            }

            int mainEffectCount = 0;
            int modifierCount = 0;
            std::string mainEffectId = EffectId::None;

            for (const auto& ingredient : ingredients){
                if (ingredient.role == IngredientRole::MainEffect){
                    ++mainEffectCount;

                    if (ingredient.effectId == EffectId::None || ingredient.effectId.empty()){
                        result.reason = "Main effect ingredient has no effect id.";
                        return result;
                    }

                    if (mainEffectId == EffectId::None){
                        mainEffectId = ingredient.effectId;
                    }

                    continue;
                }

                if (ingredient.role == IngredientRole::Modifier){
                    ++modifierCount;

                    if (ingredient.modifierId == ModifierId::None || ingredient.modifierId.empty()){
                        result.reason = "Modifier ingredient has no modifier id.";
                        return result;
                    }

                    if (ingredient.value <= 0.0f){
                        result.reason = "Modifier ingredient has invalid value.";
                        return result;
                    }

                    continue;
                }

                result.reason = "Ingredient has no valid crafting role.";
                return result;
            }

            if (mainEffectCount <= 0){
                result.reason = "At least one main effect ingredient is required.";
                return result;
            }

            result.valid = true;
            result.recipeName = BuildRecipeName(mainEffectId, modifierCount);
            result.reason = "Recipe is valid.";

            return result;
        }

        static bool HasValidRecipe(const std::vector<IngredientData>& ingredients){
            return Check(ingredients).valid;
        }

        static std::string GetRecipeName(const std::vector<IngredientData>& ingredients){
            return Check(ingredients).recipeName;
        }

        static bool IsVodkaRecipe(const std::vector<IngredientType>& ingredients){
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

    private:
        static std::string BuildRecipeName(const std::string& mainEffectId, int modifierCount){
            std::string name = mainEffectId + " Potion";

            if (modifierCount > 0){
                name += " + " + std::to_string(modifierCount) + " modifier";

                if (modifierCount > 1){
                    name += "s";
                }
            }

            return name;
        }
    };
}