#pragma once

#include <glm/glm.hpp>

class AudioListener
{
public:
    static void SetPosition(float x, float y, float z);
    static void SetPosition(const glm::vec3& position);

    static void SetVelocity(float x, float y, float z);
    static void SetVelocity(const glm::vec3& velocity);

    static void SetOrientation(const glm::vec3& forward, const glm::vec3& up);

    static void SetGain(float gain);
};