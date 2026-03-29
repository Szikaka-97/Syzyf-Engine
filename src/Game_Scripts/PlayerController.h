#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <InputSystem.h>
#include <Graphics.h>
#include <Camera.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <cmath>

class PlayerController : public GameObject, public ImGuiDrawable {
private:
    float moveSpeed = 0.08f;

public:
    void Update(){
        glm::vec3 movement = glm::vec3(0.0f);

        Camera* camera = GetScene()->GetGraphics()->GetMainCamera();
        if (!camera) return;

        glm::vec3 cameraRight = camera->GlobalTransform().Right();
        glm::vec3 cameraForward = camera->GlobalTransform().Forward();

        cameraRight.y = 0.0f;
        cameraForward.y = 0.0f;

        if (glm::length(cameraRight) > 0.0f) {
            cameraRight = glm::normalize(cameraRight);
        }
        if (glm::length(cameraForward) > 0.0f) {
            cameraForward = glm::normalize(cameraForward);
        }

        if (GetScene()->Input()->KeyPressed(Key::W)) {
            movement += cameraForward;
        }
        if (GetScene()->Input()->KeyPressed(Key::S)) {
            movement -= cameraForward;
        }
        if (GetScene()->Input()->KeyPressed(Key::A)) {
            movement += cameraRight;
        }
        if (GetScene()->Input()->KeyPressed(Key::D)) {
            movement -= cameraRight;
        }

        if (glm::length(movement) > 0.0f) {
            movement = glm::normalize(movement);
            GlobalTransform().Position() += movement * moveSpeed;

            float angle = std::atan2(movement.x, movement.z);
            GlobalTransform().Rotation() = glm::angleAxis(angle, glm::vec3(0, 1, 0));
        }
    }

    void DrawImGui() override {
        ImGui::InputFloat("Player move speed", &moveSpeed);
    }
};