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
            std::string primaryEffectId = EffectId::None;
            std::string secondaryEffectId = EffectId::None;

            for (const auto& ingredient : ingredients){
                if (ingredient.role == IngredientRole::MainEffect){
                    ++mainEffectCount;

                    if (ingredient.effectId == EffectId::None || ingredient.effectId.empty()){
                        result.reason = "Main effect ingredient has no effect id.";
                        return result;
                    }

                    if (primaryEffectId == EffectId::None){
                        primaryEffectId = ingredient.effectId;
                    }
                    else if (secondaryEffectId == EffectId::None && ingredient.effectId != primaryEffectId){
                        secondaryEffectId = ingredient.effectId;
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
            result.recipeName = BuildRecipeName(primaryEffectId, secondaryEffectId, modifierCount);
            result.reason = "Recipe is valid.";

            return result;
        }

        static bool HasValidRecipe(const std::vector<IngredientData>& ingredients){
            return Check(ingredients).valid;
        }

        static std::string GetRecipeName(const std::vector<IngredientData>& ingredients){
            return Check(ingredients).recipeName;
        }

        static CraftedPotionData BuildCraftedPotion(
            const std::vector<IngredientData>& ingredients,
            float qualityPercent
        ){
            CraftedPotionData potion;

            CraftingRecipeCheckResult checkResult = Check(ingredients);
            potion.recipeName = checkResult.recipeName;
            potion.qualityPercent = ClampQuality(qualityPercent);

            float qualityMultiplier = 0.5f + potion.qualityPercent / 100.0f;

            potion.radius = 3.0f;
            potion.duration = 4.0f;
            potion.power = 25.0f;

            for (const auto& ingredient : ingredients){
                if (ingredient.role == IngredientRole::MainEffect){
                    potion.mainEffectCount++;

                    if (potion.primaryEffectId == EffectId::None){
                        potion.primaryEffectId = ingredient.effectId;
                    }
                    else if (potion.secondaryEffectId == EffectId::None && ingredient.effectId != potion.primaryEffectId){
                        potion.secondaryEffectId = ingredient.effectId;
                    }

                    continue;
                }

                if (ingredient.role == IngredientRole::Modifier){
                    potion.modifierCount++;

                    AppendOptionalIngredientText(
                        potion.optionalIngredientsText,
                        ingredient.displayName.empty()
                            ? ingredient.modifierId
                            : ingredient.displayName
                    );

                    ApplyModifier(potion,ingredient);
                }
            }

            potion.radius *= qualityMultiplier;
            potion.duration *= qualityMultiplier;
            potion.power *= qualityMultiplier;

            if (potion.radius < 1.0f){
                potion.radius = 1.0f;
            }

            if (potion.duration < 1.0f){
                potion.duration = 1.0f;
            }

            if (potion.power < 1.0f){
                potion.power = 1.0f;
            }

            return potion;
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
        static float ClampQuality(float qualityPercent){
            if (qualityPercent < 0.0f){
                return 0.0f;
            }

            if (qualityPercent > 100.0f){
                return 100.0f;
            }

            return qualityPercent;
        }

        static void AppendOptionalIngredientText(
            std::string& optionalIngredientsText,
            const std::string& ingredientName
        ){
            if (ingredientName.empty() || ingredientName == ModifierId::None){
                return;
            }

            if (!optionalIngredientsText.empty()){
                optionalIngredientsText += ", ";
            }

            optionalIngredientsText += ingredientName;
        }

        static void ApplyModifier(CraftedPotionData& potion, const IngredientData& ingredient){
            if (ingredient.modifierId == ModifierId::Radius){
                potion.radius += ingredient.value;
                return;
            }

            if (ingredient.modifierId == ModifierId::Duration){
                potion.duration += ingredient.value;
                return;
            }

            if (ingredient.modifierId == ModifierId::Power){
                potion.power += ingredient.value;
                return;
            }
        }

        static std::string BuildRecipeName(
            const std::string& primaryEffectId,
            const std::string& secondaryEffectId,
            int modifierCount
        ){
            std::string name = primaryEffectId;

            if (secondaryEffectId != EffectId::None){
                name += " + " + secondaryEffectId;
            }

            name += " Potion";

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
