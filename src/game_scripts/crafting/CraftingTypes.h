#pragma once

#include <string>

namespace Crafting
{
  enum class IngredientType
  {
    None = 0,
    Sugar,
    Water
  };

  struct IngredientData
  {
    IngredientType type = IngredientType::None;
    std::string displayName;
  };

  inline const char* ToString(IngredientType type)
  {
    switch (type)
    {
    case IngredientType::Sugar:
      return "Sugar";
    case IngredientType::Water:
      return "Water";
    case IngredientType::None:
    default:
      return "None";
    }
  }
}