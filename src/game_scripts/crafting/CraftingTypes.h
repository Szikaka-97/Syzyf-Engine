#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Crafting{
    enum class IngredientType{
          None = 0,
          Sugar,
          Water
    };

    enum class IngredientRole{
          None = 0,
          MainEffect,
          Modifier
    };

    namespace EffectId{
        inline const std::string None = "None";
        inline const std::string Burn = "Burn";
        inline const std::string Lightning = "Lightning";
    }

    namespace ModifierId{
        inline const std::string None = "None";
        inline const std::string Radius = "Radius";
        inline const std::string Duration = "Duration";
        inline const std::string Power = "Power";
    }

    struct IngredientData{
        IngredientType type = IngredientType::None;
        std::string displayName;

        IngredientRole role = IngredientRole::None;

        std::string effectId = EffectId::None;
        std::string modifierId = ModifierId::None;

        float value = 1.0f;

        glm::vec4 color = glm::vec4(1.0f);
    };

    inline bool IsMainEffectIngredient(const IngredientData& ingredient){
          return ingredient.role == IngredientRole::MainEffect;
    }

    inline bool IsModifierIngredient(const IngredientData& ingredient){
          return ingredient.role == IngredientRole::Modifier;
    }

    inline const char* ToString(IngredientType type){
          switch (type){
              case IngredientType::Sugar:
                      return "Sugar";

              case IngredientType::Water:
                      return "Water";

              case IngredientType::None:
                      default:
                      return "None";
            }
    }

    inline const char* ToString(IngredientRole role){
          switch (role){
              case IngredientRole::MainEffect:
                    return "MainEffect";

              case IngredientRole::Modifier:
                    return "Modifier";

              case IngredientRole::None:
                    default:
                    return "None";
          }
    }
}