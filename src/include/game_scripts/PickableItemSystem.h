#pragma once

#include "GameObjectSystem.h"
#include "game_scripts/PickableItem.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

class PickableItemSystem : public GameObjectSystem<PickableItem> {
public:
    PickableItemSystem(Scene* scene) : GameObjectSystem<PickableItem>(scene) {}

    PickableItem* GetClosestItem(const glm::vec3& position, float maxRadius) {
        PickableItem* closestItem = nullptr;
        float closestDistanceSquared = maxRadius * maxRadius;

        for (PickableItem* item : this->IterateObjects()) {
            float distanceSquared = glm::distance2(position, item->GlobalTransform().Position().Value());

            if (distanceSquared < closestDistanceSquared) {
                closestDistanceSquared = distanceSquared;
                closestItem = item;
            }
        }

        return closestItem;
    }
};
