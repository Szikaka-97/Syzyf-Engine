#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/vector_angle.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

#include <optional>

namespace Editor {

class Context;

class KeyboardControls {
  public:
    enum class Mode {
        None,
        Translation,
        Rotation,
        Scale,
    };

  private:
    // Add none, and maybe change it so it supports two at the same time
    enum class Axis {
        None,
        X,
        Y,
        Z,
    };

    Axis currentAxis = Axis::None;
    Mode currentMode = Mode::None;

    std::optional<glm::vec2> initialMousePosition = std::nullopt;
    glm::quat savedRotation = glm::identity<glm::quat>();
    glm::vec3 savedScale = {1.0f, 1.0f, 1.0f};

  public:
    void Run(Context& context);

    void HandleTranslationInput(Context& context);
    void HandleRotationInput(Context& context);
    void HandleScaleInput(Context& context);
    void SwitchInputMode(Context& context);
    void SwitchAxis();

    Mode GetCurrentMode() const { return this->currentMode; }

    bool IsActive();
};
} // namespace Editor
