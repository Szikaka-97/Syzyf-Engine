#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"

#include "game_scripts/CameraSettings.h"

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace Crafting{
    class CraftingStation : public GameObject, public ImGuiDrawable{
      public:
        float interactionRadius = 3.0f;

        glm::vec3 stationCameraPosition = glm::vec3(4.0f, 2.0f, 0.0f);
        glm::quat stationCameraRotation =
            glm::quat(glm::radians(glm::vec3(20.0f, -90.0f, 0.0f)));

      private:
        bool isActive = false;

      public:
        void Awake(){
            spdlog::info("CraftingStation loaded.");
        }

        void Update(){
            if (!GetScene() || !GetScene()->Input()){
                return;
            }

            if (!isActive){
                if (IsPlayerNear()){
                    if (GetScene()->Input()->KeyDown(Key::E)){
                        EnterStation();
                    }
                }

                return;
            }

            if (GetScene()->Input()->KeyDown(Key::Escape)){
                ExitStation();
            }
        }

        void DrawImGui(){
            if (!isActive && IsPlayerNear()){
                ImGui::Begin("Interaction");
                ImGui::Text("Press E to use crafting station");
                ImGui::End();
            }

            if (isActive){
                ImGui::Begin("Crafting Station");
                ImGui::Text("Crafting station active");
                ImGui::Text("Press ESC to exit");
                ImGui::Separator();
                ImGui::Text("Ingredients and bottles will be added here later.");
                ImGui::End();
            }
        }

      private:
        SceneNode* GetPlayerNode(){
            return GetScene()->FindNode("Player");
        }

        SceneNode* GetCameraNode(){
            return GetScene()->FindNode("Camera Node");
        }

        bool IsPlayerNear(){
            if (!GetScene() || !GetNode()){
                return false;
            }

            SceneNode* playerNode = GetPlayerNode();

            if (!playerNode){
                return false;
            }

            glm::vec3 playerPos =
                playerNode->GlobalTransform().Position().Value();

            glm::vec3 stationPos =
                GetNode()->GlobalTransform().Position().Value();

            return glm::distance(playerPos, stationPos) <= interactionRadius;
        }

        void EnterStation(){
            SceneNode* cameraNode = GetCameraNode();

            if (!cameraNode){
                spdlog::warn("CraftingStation: Camera Node not found.");
                return;
            }

            Camera* camera = cameraNode->GetObject<Camera>();

            if (!camera){
                spdlog::warn("CraftingStation: Camera component not found on Camera Node.");
                return;
            }

            if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                cameraSettings->enabled = false;
            }

            cameraNode->GlobalTransform().Position() =
                stationCameraPosition;

            cameraNode->GlobalTransform().Rotation() =
                stationCameraRotation;

            camera->SetAsMainCamera();

            isActive = true;

            GetScene()->Input()->SetMouseLocked(false);

            spdlog::info("CraftingStation: entered station view.");
        }

        void ExitStation(){
            SceneNode* cameraNode = GetCameraNode();

            if (!cameraNode){
                spdlog::warn("CraftingStation: Camera Node not found.");
                return;
            }

            if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                cameraSettings->enabled = true;
            }

            isActive = false;

            spdlog::info("CraftingStation: exited station view.");
        }
    };
}