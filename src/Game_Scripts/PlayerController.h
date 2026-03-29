#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <InputSystem.h>
#include <Graphics.h>
#include <Camera.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <cmath>

class PlayerController : public GameObject, public ImGuiDrawable {
private:
    float moveSpeed = 0.08f;

    glm::vec3 GetMousePointOnGround(Camera* camera) {
        glm::vec2 mousePos = GetScene()->Input()->GetMousePosition();
        glm::vec2 screenSize = GetScene()->GetGraphics()->GetScreenResolution();

        float x = (2.0f * mousePos.x) / screenSize.x - 1.0f;
        float y = 1.0f - (2.0f * mousePos.y) / screenSize.y;

        glm::vec4 rayStartNdc(x, y, -1.0f, 1.0f);
        glm::vec4 rayEndNdc(x, y, 1.0f, 1.0f);

        glm::mat4 invVP = glm::inverse(camera->ProjectionMatrix() * camera->ViewMatrix());

        glm::vec4 rayStartWorld = invVP * rayStartNdc;
        glm::vec4 rayEndWorld = invVP * rayEndNdc;

        rayStartWorld /= rayStartWorld.w;
        rayEndWorld /= rayEndWorld.w;

        glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
        glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

        if (std::abs(rayDir.y) < 0.0001f) {
            return GlobalTransform().Position().Value();
        }

        float t = -rayOrigin.y / rayDir.y;
        return rayOrigin + rayDir * t;
    }

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
        }

        glm::vec3 mouseWorld = GetMousePointOnGround(camera);
        glm::vec3 toMouse = mouseWorld - GlobalTransform().Position().Value();
        toMouse.y = 0.0f;

        if (glm::length(toMouse) > 0.001f) {
            toMouse = glm::normalize(toMouse);
            float angle = std::atan2(toMouse.x, toMouse.z);
            GlobalTransform().Rotation() = glm::angleAxis(angle, glm::vec3(0, 1, 0));
        }
    }

    void DrawImGui() override {
        ImGui::InputFloat("Player move speed", &moveSpeed);
    }
};