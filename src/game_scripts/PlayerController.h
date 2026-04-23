#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <InputSystem.h>
#include <Graphics.h>
#include <Camera.h>
#include <game_scripts/ThrowBottle.h>
#include <physics/VirtualCharacterController.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterBase.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <cmath>

class PlayerController : public GameObject, public ImGuiDrawable {
private:
    float moveSpeed = 12.0f;
    float jumpSpeed = 8.0f;

    SceneNode* markerNode = nullptr;
    ThrowBottle* bottleThrower = nullptr;

    Physics::VirtualCharacterController* virtualController = nullptr;
    glm::vec3 velocity = glm::vec3(0.0f);

    glm::vec3 GetMousePointOnGround(Camera* camera) {
        glm::vec2 mousePos = GetScene()->Input()->GetMousePosition();
        glm::vec2 screenSize = GetScene()->GetGraphics()->GetScreenResolution();

        if (screenSize.x <= 0.0f || screenSize.y <= 0.0f) {
            return GlobalTransform().Position().Value();
        }

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

    void TryInitController() {
        if (!virtualController) {
            virtualController = GetObject<Physics::VirtualCharacterController>();
            virtualController->SetCollisionLayerAndMask({1}, {0});
        }
    }

public:
    PlayerController(SceneNode* markerNode = nullptr) : markerNode(markerNode) {}

    void SetBottleThrower(ThrowBottle* thrower) {
        bottleThrower = thrower;
    }

    void Update() {
        TryInitController();
        if (!virtualController) return;

        Camera* camera = GetScene()->GetGraphics()->GetMainCamera();
        if (!camera) return;

        glm::vec3 movement(0.0f);
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
        }

        velocity.x = movement.x * moveSpeed;
        velocity.z = movement.z * moveSpeed;

        if (virtualController->IsSupported()) {
            velocity.y = 0.0f;

            if (GetScene()->Input()->KeyPressed(Key::Space) &&
                virtualController->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround) {
                velocity.y = jumpSpeed;
            }
        } else {
            velocity.y += -9.81f * virtualController->GetGravityFactor() * (1.0f / 60.0f);
        }

        virtualController->Move(velocity, 1.0f / 60.0f);

        GlobalTransform().Position() = virtualController->GetPosition();

        glm::vec3 mouseWorld = GetMousePointOnGround(camera);

        if (markerNode) {
            glm::vec3 markerPos = mouseWorld;
            markerPos.y += 0.02f;
            markerNode->GlobalTransform().Position() = markerPos;
        }

        glm::vec3 toMouse = mouseWorld - GlobalTransform().Position().Value();
        toMouse.y = 0.0f;

        if (glm::length(toMouse) > 0.001f) {
            toMouse = glm::normalize(toMouse);
            float angle = std::atan2(toMouse.x, toMouse.z);
            GlobalTransform().Rotation() = glm::angleAxis(angle, glm::vec3(0, 1, 0));
            virtualController->SetRotation(GlobalTransform().Rotation().Value());
        }

        if (GetScene()->Input()->ButtonDown(MouseButton::Left)) {
            if (bottleThrower) {
                glm::vec3 startPos = GlobalTransform().Position().Value() + glm::vec3(0.0f, 1.2f, 0.0f);
                bottleThrower->LaunchBottle(startPos, mouseWorld, 0.8f, 3.0f);
            }
        }
    }

    void DrawImGui() override {
        ImGui::InputFloat("Player move speed", &moveSpeed);
        ImGui::InputFloat("Jump speed", &jumpSpeed);
    }
};
