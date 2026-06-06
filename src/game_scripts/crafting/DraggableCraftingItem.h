#pragma once

#include "GameObject.h"

#include "game_scripts/crafting/CraftingTypes.h"

#include <physics/Body.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

namespace Crafting{
  class DraggableCraftingItem : public GameObject {
    public:
      IngredientData data;

      bool isDragged = false;
      bool returnToStartOnInvalidDrop = true;

      float moveLerpSpeed = 0.18f;
      float finishDistance = 0.03f;

      void Awake()
      {
        startPosition = GetNode()->GlobalTransform().Position().Value();

        spdlog::info("Draggable item ready: {}", data.displayName);
      }

      void Update()
      {
        if (isDragged)
        {
          return;
        }

        if (motionMode == MotionMode::None)
        {
          return;
        }

        glm::vec3 currentPosition =
          GetNode()->GlobalTransform().Position().Value();

        glm::vec3 toTarget =
          motionTargetPosition - currentPosition;

        float distance =
          glm::length(toTarget);

        if (distance <= finishDistance)
        {
          SetWorldPosition(motionTargetPosition);

          MotionMode finishedMode = motionMode;
          motionMode = MotionMode::None;

          if (finishedMode == MotionMode::Consume)
          {
            spdlog::info("Consumed {}.", data.displayName);
            GetNode()->SetEnabled(false);
          }

          if (finishedMode == MotionMode::ReturnToStart)
          {
            spdlog::info("Returned {} to start position.", data.displayName);
          }

          return;
        }

        glm::vec3 nextPosition =
          currentPosition + toTarget * moveLerpSpeed;

        SetWorldPosition(nextPosition);
      }

      void BeginDrag()
      {
        motionMode = MotionMode::None;

        isDragged = true;

        SyncPhysicsToNode();

        spdlog::info("Picked up {}.", data.displayName);
      }

      void DragTo(const glm::vec3& position)
      {
        if (!isDragged){return;}

        SetWorldPosition(position);
      }

      void EndDrag()
      {
        if (!isDragged){return;}

        SyncPhysicsToNode();

        isDragged = false;

        spdlog::info("Released {}.", data.displayName);
      }

      void ReturnToStart()
      {
        motionTargetPosition = startPosition;
        motionMode = MotionMode::ReturnToStart;

        spdlog::info("Returning {} to start position.", data.displayName);
      }

      void Consume()
      {
        motionMode = MotionMode::None;

        spdlog::info("Consumed {}.", data.displayName);

        GetNode()->SetEnabled(false);
      }

      void ConsumeAt(const glm::vec3& position)
      {
        motionTargetPosition = position;
        motionMode = MotionMode::Consume;

        spdlog::info(
          "Moving {} to cauldron consume position.",
          data.displayName
        );
      }

      glm::vec3 GetStartPosition() const
      {
        return startPosition;
      }

      void ResetForNewSession()
      {
        isDragged = false;
        motionMode = MotionMode::None;

        if (GetNode())
        {
          GetNode()->SetEnabled(true);
          SetWorldPosition(startPosition);
        }

        spdlog::info("Reset draggable ingredient: {}.", data.displayName);
      }

    private:
      enum class MotionMode
      {
        None = 0,
        ReturnToStart,
        Consume
      };

      glm::vec3 startPosition = glm::vec3(0.0f);
      glm::vec3 motionTargetPosition = glm::vec3(0.0f);

      MotionMode motionMode = MotionMode::None;

      void SetWorldPosition(const glm::vec3& position)
      {
        GetNode()->GlobalTransform().Position() = position;

        SyncPhysicsToNode();
      }

      void SyncPhysicsToNode()
      {
        if (auto* body = GetNode()->GetObject<Physics::Body>())
        {
          body->SyncToNode();

          body->SetLinearVelocity(glm::vec3(0.0f));
          body->SetAngularVelocity(glm::vec3(0.0f));
        }
      }
    };
}