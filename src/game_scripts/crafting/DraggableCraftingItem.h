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
      bool returnToStartOnInvalidDrop = true;

      void Awake()
      {
        startPosition = GetNode()->GlobalTransform().Position().Value();

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

        SetWorldPosition(position);
      }

      void EndDrag(){
        if (!isDragged){return;}

        isDragged = false;
        spdlog::info("Released {}.", data.displayName);
      }

      void ReturnToStart(){
        SetWorldPosition(startPosition);

        spdlog::info("Returned {} to start position.", data.displayName);
      }

      void Consume(){
        spdlog::info("Consumed {}.", data.displayName);

        GetNode()->SetEnabled(false);
      }

      glm::vec3 GetStartPosition() const{
        return startPosition;
      }

    private:
      glm::vec3 startPosition = glm::vec3(0.0f);

      void SetWorldPosition(const glm::vec3& position){
        GetNode()->GlobalTransform().Position() = position;

        if (auto* body = GetNode()->GetObject<Physics::Body>()){
          body->SetPosition(position);
        }
      }
    };
}