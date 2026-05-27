#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"

#include "game_scripts/CameraSettings.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"
#include "game_scripts/crafting/CraftingRecipeChecker.h"

#include <TimeSystem.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace Crafting{
    class CraftingStation : public GameObject, public ImGuiDrawable{
          private:
                bool isActive = false;
                bool playerWasEnabled = true;

                bool recipeCompleted = false;

                CraftingIngredientReceiver* receiver = nullptr;

          public:
                float interactionRadius = 3.0f;
                bool isProcessing = false;
                float processTimer = 0.0f;

                glm::vec3 stationCameraPosition = glm::vec3(4.0f, 2.0f, 0.0f);

                glm::quat stationCameraRotation =
                    glm::quat(glm::radians(glm::vec3(20.0f, -90.0f, 0.0f)));

                void Awake(){
                    spdlog::info("CraftingStation loaded.");

                    SceneNode* cauldronNode =
                        GetNode()->FindNode("Cauldron");

                    if (!cauldronNode){
                        spdlog::error("CraftingStation: Cauldron node not found.");
                        return;
                    }

                    receiver =
                        cauldronNode->GetObject<CraftingIngredientReceiver>();

                    if (!receiver){
                        spdlog::error("CraftingStation: CraftingIngredientReceiver missing.");
                    }
                }

                void Update(){
                    if (!GetScene() || !GetScene()->Input()){return;}

                    if (isProcessing){
                        processTimer -= Time::Delta();

                        RotateLid();

                        if (processTimer <= 0.0f){
                            isProcessing = false;
                            processTimer = 0.0f;
                            recipeCompleted = false;

                            if (receiver){
                                receiver->Clear();
                            }

                            spdlog::info("Craft completed!");
                        }
                    }

                    if (!isActive){
                        if (IsPlayerNear()){
                            if (GetScene()->Input()->KeyDown(Key::E)){
                                EnterStation();
                            }
                        }

                        return;
                    }

                    if (!recipeCompleted && !isProcessing && receiver){
                        if (CraftingRecipeChecker::IsVodkaRecipe(
                                receiver->insertedIngredients)){
                            recipeCompleted = true;
                            StartProcessing();
                        }
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

                        if (receiver){
                            ImGui::Text(
                                "Ingredients inserted: %d",
                                (int)receiver->insertedIngredients.size());
                        }

                        if (recipeCompleted){
                            ImGui::Text("Recipe detected: Vodka");
                        }

                        if (isProcessing){
                            ImGui::Text(
                                "Processing... %.2f",
                                processTimer);
                        }

                        ImGui::End();
                    }
                }

          private:
                void StartProcessing(){
                    isProcessing = true;
                    processTimer = 5.0f;

                    FocusCameraOnMachine();

                    spdlog::info("Craft started!");
                }

                void RotateLid(){
                    RotateNode("Lid");
                }

                void RotateNode(const std::string& nodeName){
                    SceneNode* node =
                        GetNode()->FindNode(nodeName);

                    if (!node){
                        return;
                    }

                    node->LocalTransform().Rotation() =
                        glm::quat(
                            glm::radians(
                                glm::vec3(0.0f, 0.0f, -90.0f)
                            )
                        );
                }

                void FocusCameraOnMachine(){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        return;
                    }

                    cameraNode->GlobalTransform().Position() =
                        glm::vec3(-4.0f, 1.5f, -2.0f);

                    cameraNode->GlobalTransform().Rotation() =
                        glm::quat(
                            glm::radians(
                                glm::vec3(15.0f, 0.0f, 0.0f)
                            )
                        );
                }

                SceneNode* GetPlayerNode(){
                    return GetScene()->FindNode("Player");
                }

                SceneNode* GetCameraNode(){
                    return GetScene()->FindNode("Camera Node");
                }

                SceneNode* GetIngredientsRootNode(){
                    return GetScene()->FindNode("Crafting Root/Root/Crafting Ingredients");
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

                void SetIngredientsEnabled(bool enabled){
                    SceneNode* ingredientsRootNode = GetIngredientsRootNode();

                    if (ingredientsRootNode){
                        ingredientsRootNode->SetEnabled(enabled);
                    }
                }

                void DisablePlayer(){
                    SceneNode* playerNode = GetPlayerNode();

                    if (!playerNode){
                        return;
                    }

                    playerWasEnabled = playerNode->EnabledSelf();

                    playerNode->SetEnabled(false);
                }

                void EnablePlayer(){
                    SceneNode* playerNode = GetPlayerNode();

                    if (!playerNode){
                        return;
                    }

                    playerNode->SetEnabled(playerWasEnabled);
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

                    DisablePlayer();
                    SetIngredientsEnabled(true);

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

                    EnablePlayer();
                    SetIngredientsEnabled(false);

                    if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                        cameraSettings->enabled = true;
                    }

                    isActive = false;
                    recipeCompleted = false;

                    GetScene()->Input()->SetMouseLocked(true);

                    spdlog::info("CraftingStation: exited station view.");
                }
    };
}