#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

class ThrowBottle : public GameObject
{
private:
    glm::vec3 startPosition;
    glm::vec3 targetPosition;

    float flightDuration;
    float arcHeight;
    float timer;

    bool landed = false;
    float landedTimer = 0.0f;

public:
    ThrowBottle(glm::vec3 startPosition, glm::vec3 targetPosition,
                float flightDuration = 0.8f, float arcHeight = 3.0f)
        : startPosition(startPosition),
          targetPosition(targetPosition),
          flightDuration(flightDuration),
          arcHeight(arcHeight),
          timer(0.0f)
    {
    }

    void Update() {
        timer += 1.0f / 60.0f;

        float t = std::clamp(timer / flightDuration, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(startPosition, targetPosition, t);

        float arc = 4.0f * arcHeight * t * (1.0f - t);
        pos.y += arc;

        GlobalTransform().Position() = pos;
        GlobalTransform().Rotation() *= glm::angleAxis(glm::radians(12.0f), glm::vec3(1, 0, 0));

        if (t >= 1.0f || pos.y <= 0.0f)
        {
            delete GetNode();
        }
    }
};