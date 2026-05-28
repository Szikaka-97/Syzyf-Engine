#pragma once

#include "GameObject.h"

#include "game_scripts/crafting/CraftingTypes.h"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <vector>

namespace Crafting{
    class Cauldron : public GameObject{
    public:
          int maxMainEffectIngredients = 2;
          int maxModifierIngredients = 2;

          bool AddIngredient(const IngredientData& ingredient){
                if (!CanAddIngredient(ingredient)){
                    spdlog::warn(
                        "Cauldron: cannot add ingredient {} with role {}.",
                        ingredient.displayName,
                        ToString(ingredient.role)
                    );

                    return false;
                }

                ingredients.push_back(ingredient);

                spdlog::info(
                    "Cauldron: added ingredient {}.",
                    ingredient.displayName
                );

                return true;
          }

          bool CanAddIngredient(const IngredientData& ingredient) const{
                if (ingredient.role == IngredientRole::MainEffect){
                    return CountMainEffectIngredients() < maxMainEffectIngredients;
                }

                if (ingredient.role == IngredientRole::Modifier){
                    return CountModifierIngredients() < maxModifierIngredients;
                }

                return false;
          }

          bool CanConfirm() const{
                return CountMainEffectIngredients() > 0;
          }

          bool HasAnyIngredient() const{
                return !ingredients.empty();
          }

          void Clear(){
                ingredients.clear();

                spdlog::info("Cauldron: cleared ingredients.");
          }

          int CountMainEffectIngredients() const{
                int count = 0;

                for (const auto& ingredient : ingredients){
                    if (ingredient.role == IngredientRole::MainEffect){
                        count++;
                    }
                }

                return count;
          }

          int CountModifierIngredients() const{
                int count = 0;

                for (const auto& ingredient : ingredients){
                    if (ingredient.role == IngredientRole::Modifier){
                        count++;
                    }
                }

                return count;
          }

          const std::vector<IngredientData>& GetIngredients() const{
                return ingredients;
          }

          glm::vec4 CalculateLiquidColor() const{
                if (ingredients.empty()){
                    return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                }

                glm::vec4 result = glm::vec4(0.0f);

                for (const auto& ingredient : ingredients){
                    result += ingredient.color;
                }

                result /= static_cast<float>(ingredients.size());
                result.a = 1.0f;

                return result;
          }

    private:
        std::vector<IngredientData> ingredients;
    };
}