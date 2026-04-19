#include "KeyboardControls.h"

#include "Application.h"

#include <Scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/vector_angle.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

namespace Editor {

void KeyboardControls::Run(Context& context) {
    SwitchInputMode(context);
    SwitchAxis();

    switch (this->currentMode) {
    case Mode::None:
        break;
    case Mode::Translation:
        HandleTranslationInput(context);
        break;
    case Mode::Rotation:
        HandleRotationInput(context);
        break;
    case Mode::Scale:
        HandleScaleInput(context);
        break;
    }
}

void KeyboardControls::HandleTranslationInput(Context& context) { return; }

void KeyboardControls::HandleRotationInput(Context& context) {
    ImVec2 windowPosition = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    glm::vec2 screenCenter(windowPosition.x + windowSize.x * 0.5f,
                           windowPosition.y + windowSize.y * 0.5f);
    glm::vec2 mousePosition(ImGui::GetMousePos().x, ImGui::GetMousePos().y);

    if (!this->initialMousePosition.has_value()) {
        this->initialMousePosition = mousePosition;
        this->savedRotation =
            context.selectedNode->LocalTransform().Rotation().value;
    }

    glm::vec2 toInitialMousePosition =
        glm::normalize(this->initialMousePosition.value() - screenCenter);
    glm::vec2 toMousePosition = glm::normalize(mousePosition - screenCenter);

    float angle = -glm::orientedAngle(toInitialMousePosition, toMousePosition);

    switch (this->currentAxis) {
    case Axis::None:
        // TODO
        break;
    case Axis::X:
        context.selectedNode->LocalTransform().Rotation() =
            glm::angleAxis(angle, glm::vec3(1.0f, 0.0f, 0.0f)) *
            this->savedRotation;
        break;
    case Axis::Y:
        context.selectedNode->LocalTransform().Rotation() =
            glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f)) *
            this->savedRotation;
        break;
    case Axis::Z:
        context.selectedNode->LocalTransform().Rotation() =
            glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f)) *
            this->savedRotation;
        break;
    }
}

void KeyboardControls::HandleScaleInput(Context& context) {
    ImVec2 windowPosition = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    glm::vec2 screenCenter(windowPosition.x + windowSize.x * 0.5f,
                           windowPosition.y + windowSize.y * 0.5f);
    glm::vec2 mousePosition(ImGui::GetMousePos().x, ImGui::GetMousePos().y);

    if (!this->initialMousePosition.has_value()) {
        this->initialMousePosition = mousePosition;
        this->savedRotation =
            context.selectedNode->LocalTransform().Rotation().value;
    }

    if (!this->initialMousePosition.has_value()) {
        this->initialMousePosition = mousePosition;
        this->savedScale = context.selectedNode->LocalTransform().Scale().value;
    }

    float initialDistanceToCenter =
        glm::length(this->initialMousePosition.value() - screenCenter);
    float distanceToCenter = glm::length(mousePosition - screenCenter);

    float scaleAmount = distanceToCenter / (initialDistanceToCenter + 0.0001f);

    spdlog::info("Scale amount: {}", scaleAmount);

    switch (this->currentAxis) {
    case Axis::None:
        context.selectedNode->LocalTransform().Scale() =
            this->savedScale * glm::vec3(scaleAmount);
        break;
    case Axis::X:
        context.selectedNode->LocalTransform().Scale() =
            this->savedScale * glm::vec3(scaleAmount, 1.0f, 1.0f);
        break;
    case Axis::Y:
        context.selectedNode->LocalTransform().Scale() =
            this->savedScale * glm::vec3(1.0f, scaleAmount, 1.0f);

        break;
    case Axis::Z:
        context.selectedNode->LocalTransform().Scale() =
            this->savedScale * glm::vec3(1.0f, 1.0f, scaleAmount);
        break;
    }
}

void KeyboardControls::SwitchInputMode(Context& context) {
    // Not sure whether i should be returning or not
    if (ImGui::Shortcut(ImGuiKey_G)) {
        this->currentMode = Mode::Translation;
        return;
    }
    if (ImGui::Shortcut(ImGuiKey_R)) {
        this->currentMode = Mode::Rotation;
        return;
    }
    if (ImGui::Shortcut(ImGuiKey_S)) {
        this->currentMode = Mode::Scale;
        return;
    }

    if ((ImGui::Shortcut(ImGuiKey_Enter) ||
         ImGui::IsMouseClicked(ImGuiMouseButton_Left)) &&
        this->currentMode != Mode::None) {
        if (this->currentMode == Mode::Rotation) {
            context.selectedNode->LocalTransform().Rotation() = glm::normalize(
                context.selectedNode->LocalTransform().Rotation().value);
        }

        this->initialMousePosition = std::nullopt;
        this->currentMode = Mode::None;
        this->currentAxis = Axis::None;
        return;
    }

    if (ImGui::Shortcut(ImGuiKey_Escape) && this->currentMode != Mode::None) {
        switch (this->currentMode) {
        case Mode::Rotation:
            context.selectedNode->GetTransform().LocalTransform().Rotation() =
                this->savedRotation;
            break;
        case Mode::Scale:
            context.selectedNode->GetTransform().LocalTransform().Scale() =
                this->savedScale;
            break;
        default:
            break;
        }

        this->initialMousePosition = std::nullopt;
        this->currentMode = Mode::None;
        this->currentAxis = Axis::None;
        return;
    }
}

void KeyboardControls::SwitchAxis() {
    if (this->currentMode == Mode::None) {
        return;
    }

    if (ImGui::Shortcut(ImGuiKey_X)) {
        this->currentAxis = Axis::X;
        return;
    }
    if (ImGui::Shortcut(ImGuiKey_Y)) {
        this->currentAxis = Axis::Y;
        return;
    }
    if (ImGui::Shortcut(ImGuiKey_Z)) {
        this->currentAxis = Axis::Z;
        return;
    }
}

bool KeyboardControls::IsActive() {
    return this->currentMode == Mode::None ? true : false;
}
} // namespace Editor
