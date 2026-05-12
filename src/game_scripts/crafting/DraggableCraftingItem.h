#pragma once

#include "GameObject.h"

#include "game_scripts/crafting/CraftingTypes.h"

#include <physics/Body.h>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace Crafting{
  class DraggableCraftingItem : public GameObject {
    public:
      IngredientData data;

      bool isDragged = false;

      void Awake()
      {
        spdlog::info("Draggable item ready: {}", data.displayName);
      }

      void BeginDrag()
      {
        isDragged = true;
        spdlog::info("Picked up {}.", data.displayName);
      }

      void DragTo(const glm::vec3& position)
      {
        if (!isDragged){return;}

        GetNode()->GlobalTransform().Position() = position;

        if (auto* body = GetNode()->GetObject<Physics::Body>()){
          body->SetPosition(position);
        }
      }

      void EndDrag(){
        if (!isDragged){return;}

        isDragged = false;
        spdlog::info("Released {}.", data.displayName);
      }
  };
}