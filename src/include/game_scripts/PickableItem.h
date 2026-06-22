#pragma once

#include "GameObject.h"

#include <glm/glm.hpp>

#include <string>

class PickableItem : public GameObject {
public:
    virtual void OnPickUp() {}

    virtual bool ShouldAlwaysShowPickupOutline() const {
        return false;
    }

    virtual bool ShouldShowPickupMarkerWhenReachable() const {
        return false;
    }

    virtual std::string GetPickupMarkerModelPath() const {
        return "./res/models/Marker.glb";
    }

    virtual glm::vec3 GetPickupMarkerOffset() const {
        return glm::vec3(0.0f, 0.65f, 0.0f);
    }

    virtual glm::vec3 GetPickupMarkerScale() const {
        return glm::vec3(0.35f);
    }
};
