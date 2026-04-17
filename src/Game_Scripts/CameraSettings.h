#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

class CameraSettings : public GameObject
{
private:
    SceneNode* target;
    glm::vec3 cameraOffset;
    glm::vec3 lookOffset;

public:
    CameraSettings(SceneNode* target)
        : target(target),
          cameraOffset(glm::vec3(-20.0f, 30.0f, -20.0f)),
          lookOffset(glm::vec3(0.0f, 1.0f, 0.0f))
    {
    }

    void Update() {
        if (!target) return;

        glm::vec3 playerPosition = target->GlobalTransform().Position().Value();
        glm::vec3 targetLookPoint = playerPosition + lookOffset;
        glm::vec3 cameraPosition = playerPosition + cameraOffset;

        GlobalTransform().Position() = cameraPosition;

        glm::vec3 direction = glm::normalize(targetLookPoint - cameraPosition);

        float yaw = std::atan2(direction.x, direction.z);
        float pitch = -std::asin(direction.y);

        GlobalTransform().Rotation() =
            glm::angleAxis(yaw, glm::vec3(0, 1, 0)) *
            glm::angleAxis(pitch, glm::vec3(1, 0, 0));
    }
};