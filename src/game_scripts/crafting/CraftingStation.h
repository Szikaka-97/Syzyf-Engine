#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"

#include "game_scripts/CameraSettings.h"
#include "game_scripts/crafting/Cauldron.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"

#include <physics/Body.h>
#include <physics/System.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <string>

namespace Crafting{
    class CraftingStation : public GameObject, public ImGuiDrawable{
          private:
                class LidRayFilter : public JPH::BodyFilter{
                public:
                    explicit LidRayFilter(SceneNode* lidHitboxNode)
                        : lidHitboxNode(lidHitboxNode){}

                    bool ShouldCollide(const JPH::BodyID& inBodyID) const override{
                        return true;
                    }

                    bool ShouldCollideLocked(const JPH::Body& inBody) const override{
                        auto* object =
                            reinterpret_cast<GameObject*>(inBody.GetUserData());

                        if (!object){
                            return false;
                        }

                        SceneNode* node = object->GetNode();

                        while (node){
                            if (node == lidHitboxNode){
                                return true;
                            }

                            node = node->GetParent();
                        }

                        return false;
                    }

                private:
                    SceneNode* lidHitboxNode = nullptr;
                };

                bool isActive = false;
                bool playerWasEnabled = true;

                bool ingredientStageConfirmed = false;
                bool lidInteractionEnabled = false;

                Cauldron* cauldron = nullptr;

                SceneNode* lidNode = nullptr;
                SceneNode* lidHitboxNode = nullptr;
                SceneNode* stationHitboxNode = nullptr;

                bool lidHitboxIsChildOfLid = false;

                glm::vec3 lidClosedLocalPosition = glm::vec3(0.0f);
                glm::vec3 lidHitboxClosedLocalPosition = glm::vec3(0.0f);

          public:
                float interactionRadius = 3.0f;

                std::string lidNodeName = "Lid";
                std::string lidHitboxNodeName = "LidHitbox";
                std::string stationHitboxNodeName = "StationHitbox";

                std::string ingredientCameraPointNodeName = "IngredientCameraPoint";
                std::string ingredientCameraTargetNodeName = "IngredientCameraTarget";

                std::string heatingCameraPointNodeName = "HeatingCameraPoint";
                std::string heatingCameraTargetNodeName = "HeatingCameraTarget";

                glm::vec3 lidOpenOffset = glm::vec3(-1.5f, 0.0f, 0.0f);

                float interactionDistance = 100.0f;
                glm::vec2 viewportSize = glm::vec2(1024.0f, 576.0f);

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

                    cauldron =
                        cauldronNode->GetObject<Cauldron>();

                    if (!cauldron){
                        spdlog::error("CraftingStation: Cauldron component missing.");
                    }

                    lidNode =
                        GetNode()->FindNode(lidNodeName);

                    if (!lidNode){
                        spdlog::error("CraftingStation: Lid node not found.");
                        return;
                    }

                    lidHitboxNode =
                        lidNode->FindNode(lidHitboxNodeName);

                    if (!lidHitboxNode){
                        lidHitboxNode =
                            GetNode()->FindNode(lidHitboxNodeName);
                    }

                    if (!lidHitboxNode){
                        spdlog::error("CraftingStation: LidHitbox node not found.");
                    }else{
                        lidHitboxIsChildOfLid =
                            IsNodeChildOf(lidHitboxNode,lidNode);

                        spdlog::info("CraftingStation: LidHitbox found.");
                    }

                    stationHitboxNode =
                        GetNode()->FindNode(stationHitboxNodeName);

                    if (!stationHitboxNode){
                        spdlog::warn("CraftingStation: StationHitbox node not found.");
                    }else{
                        stationHitboxNode->SetEnabled(true);
                        SyncPhysicsBodyToNode(stationHitboxNode);

                        spdlog::info("CraftingStation: StationHitbox found.");
                    }

                    lidClosedLocalPosition =
                        lidNode->LocalTransform().Position().Value();

                    if (lidHitboxNode){
                        lidHitboxClosedLocalPosition =
                            lidHitboxNode->LocalTransform().Position().Value();
                    }

                    CloseLid();

                    SetLidInteractionEnabled(false);
                    SetStationHitboxEnabled(true);
                }

                void Update(){
                    if (!GetScene() || !GetScene()->Input()){return;}

                    if (!isActive){
                        if (IsPlayerNear()){
                            if (GetScene()->Input()->KeyDown(Key::E)){
                                EnterStation();
                            }
                        }

                        return;
                    }

                    if (!ingredientStageConfirmed){
                        if (cauldron && cauldron->CanConfirm()){
                            SetLidInteractionEnabled(true);
                        }else{
                            SetLidInteractionEnabled(false);
                        }

                        if (GetScene()->Input()->ButtonUp(MouseButton::Left)){
                            TryConfirmIngredientStageByLidClick();
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

                        if (cauldron){
                            ImGui::Text(
                                "Main ingredients: %d / %d",
                                cauldron->CountMainEffectIngredients(),
                                cauldron->maxMainEffectIngredients);

                            ImGui::Text(
                                "Modifier ingredients: %d / %d",
                                cauldron->CountModifierIngredients(),
                                cauldron->maxModifierIngredients);

                            if (cauldron->CanConfirm()){
                                ImGui::Text("Click Lid to confirm");
                            }else{
                                ImGui::Text("Add at least one main ingredient");
                            }
                        }

                        if (lidInteractionEnabled){
                            ImGui::Text("Lid interaction enabled");
                        }else{
                            ImGui::Text("Lid interaction disabled");
                        }

                        if (ingredientStageConfirmed){
                            ImGui::Text("Ingredient stage confirmed");
                        }

                        ImGui::End();
                    }
                }

          private:
                void OpenLid(){
                    if (!lidNode){return;}

                    SetLidLocalPosition(
                        lidClosedLocalPosition + lidOpenOffset
                    );

                    spdlog::info("CraftingStation: Lid opened.");
                }

                void CloseLid(){
                    if (!lidNode){return;}

                    SetLidLocalPosition(lidClosedLocalPosition);

                    spdlog::info("CraftingStation: Lid closed.");
                }

                void SetLidInteractionEnabled(bool enabled){
                    if (lidInteractionEnabled == enabled){
                        if (lidHitboxNode && lidHitboxNode->EnabledSelf() != enabled){
                            lidHitboxNode->SetEnabled(enabled);

                            if (enabled){
                                SyncPhysicsBodyToNode(lidHitboxNode);
                            }
                        }

                        return;
                    }

                    lidInteractionEnabled = enabled;

                    if (lidHitboxNode){
                        lidHitboxNode->SetEnabled(enabled);

                        if (enabled){
                            SyncPhysicsBodyToNode(lidHitboxNode);
                        }
                    }

                    spdlog::info(
                        "CraftingStation: Lid interaction {}.",
                        enabled ? "enabled" : "disabled"
                    );
                }

                void SetStationHitboxEnabled(bool enabled){
                    if (!stationHitboxNode){
                        return;
                    }

                    if (stationHitboxNode->EnabledSelf() == enabled){
                        return;
                    }

                    stationHitboxNode->SetEnabled(enabled);

                    if (enabled){
                        SyncPhysicsBodyToNode(stationHitboxNode);
                    }

                    spdlog::info(
                        "CraftingStation: StationHitbox {}.",
                        enabled ? "enabled" : "disabled"
                    );
                }

                void SetLidLocalPosition(const glm::vec3& localPosition){
                    if (!lidNode){return;}

                    glm::vec3 lidDelta =
                        localPosition - lidClosedLocalPosition;

                    lidNode->LocalTransform().Position() =
                        localPosition;

                    SyncPhysicsBodyToNode(lidNode);

                    if (lidHitboxNode){
                        if (!lidHitboxIsChildOfLid){
                            lidHitboxNode->LocalTransform().Position() =
                                lidHitboxClosedLocalPosition + lidDelta;
                        }

                        SyncPhysicsBodyToNode(lidHitboxNode);
                    }
                }

                bool IsNodeChildOf(SceneNode* node, SceneNode* expectedParent){
                    SceneNode* current = node;

                    while (current){
                        if (current == expectedParent){
                            return true;
                        }

                        current = current->GetParent();
                    }

                    return false;
                }

                void SyncPhysicsBodyToNode(SceneNode* node){
                    if (!node){return;}

                    auto* body = node->GetObject<Physics::Body>();

                    if (!body){return;}

                    if (!node->EnabledSelf()){
                        return;
                    }

                    body->SetPosition(
                        node->GlobalTransform().Position().Value()
                    );

                    body->SetRotation(
                        node->GlobalTransform().Rotation().Value()
                    );
                }

                void TryConfirmIngredientStageByLidClick(){
                    if (!WasLidClicked()){
                        return;
                    }

                    if (!cauldron){
                        spdlog::warn("CraftingStation: cannot confirm, Cauldron missing.");
                        return;
                    }

                    if (!cauldron->CanConfirm()){
                        spdlog::warn("CraftingStation: cannot confirm, no main ingredient in Cauldron.");
                        return;
                    }

                    ingredientStageConfirmed = true;

                    CloseLid();

                    SetLidInteractionEnabled(false);

                    FocusCameraOnHeatingStage();

                    spdlog::info("CraftingStation: ingredient stage confirmed by Lid click.");
                }

                bool WasLidClicked(){
                    if (!lidInteractionEnabled){
                        return false;
                    }

                    if (!lidHitboxNode){
                        return false;
                    }

                    Camera* camera = GetMainCamera();

                    if (!camera){
                        spdlog::warn("CraftingStation: main camera not found for Lid click.");
                        return false;
                    }

                    glm::vec3 rayOrigin;
                    glm::vec3 rayDirection;

                    if (!BuildMouseRay(camera,rayOrigin,rayDirection)){
                        return false;
                    }

                    auto* physics = GetScene()->GetComponent<Physics::System>();

                    if (!physics){
                        spdlog::warn("CraftingStation: Physics::System not found.");
                        return false;
                    }

                    LidRayFilter lidFilter(lidHitboxNode);

                    Physics::RayCastPayload hit =
                        physics->CastRay(
                            rayOrigin,
                            rayDirection * interactionDistance,
                            {},
                            {},
                            lidFilter
                        );

                    if (!hit.hasHit){
                        spdlog::info("CraftingStation: Lid raycast missed.");
                        return false;
                    }

                    spdlog::info("CraftingStation: Lid clicked.");
                    return true;
                }

                Camera* GetMainCamera(){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        return nullptr;
                    }

                    return cameraNode->GetObject<Camera>();
                }

                bool BuildMouseRay(Camera* camera, glm::vec3& outOrigin, glm::vec3& outDirection){
                    if (!camera || !GetScene() || !GetScene()->Input()){
                        return false;
                    }

                    glm::vec2 mousePosition =
                        GetScene()->Input()->GetMousePosition();

                    if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f){
                        spdlog::warn(
                            "CraftingStation: invalid viewport size {} x {}.",
                            viewportSize.x,
                            viewportSize.y
                        );

                        return false;
                    }

                    float ndcX = (2.0f * mousePosition.x) / viewportSize.x - 1.0f;
                    float ndcY = 1.0f - (2.0f * mousePosition.y) / viewportSize.y;

                    glm::vec4 clipSpacePosition =
                        glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

                    glm::vec4 viewSpacePosition =
                        glm::inverse(camera->ProjectionMatrix()) * clipSpacePosition;

                    viewSpacePosition.z = -1.0f;
                    viewSpacePosition.w = 0.0f;

                    glm::vec4 worldDirection =
                        glm::inverse(camera->ViewMatrix()) * viewSpacePosition;

                    outOrigin =
                        camera->GlobalTransform().Position().Value();

                    outDirection =
                        glm::normalize(glm::vec3(worldDirection));

                    return true;
                }

                glm::quat LookAtRotation(const glm::vec3& cameraPosition,const glm::vec3& targetPosition){
                    glm::vec3 direction =
                        targetPosition - cameraPosition;

                    if (glm::length(direction) < 0.0001f){
                        spdlog::warn("CraftingStation: camera point and target are too close. Using fallback rotation.");
                        return stationCameraRotation;
                    }

                    glm::vec3 forward =
                        glm::normalize(direction);

                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

                    if (glm::abs(glm::dot(forward,up)) > 0.98f){
                        up = glm::vec3(0.0f, 0.0f, 1.0f);
                    }

                    glm::mat4 view =
                        glm::lookAt(
                            cameraPosition,
                            targetPosition,
                            up
                        );

                    return glm::quat_cast(glm::inverse(view)) *
                        glm::quat(glm::radians(glm::vec3(0.0f, 180.0f, 0.0f)));
                }

                void FocusCameraOnIngredientStage(){
                    FocusCameraOnStage(
                        ingredientCameraPointNodeName,
                        ingredientCameraTargetNodeName,
                        "ingredient"
                    );
                }

                void FocusCameraOnHeatingStage(){
                    FocusCameraOnStage(
                        heatingCameraPointNodeName,
                        heatingCameraTargetNodeName,
                        "heating"
                    );
                }

                void FocusCameraOnStage(
                    const std::string& pointNodeName,
                    const std::string& targetNodeName,
                    const std::string& stageName
                ){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        spdlog::warn("CraftingStation: Camera Node missing in FocusCameraOnStage.");
                        return;
                    }

                    SceneNode* cameraPointNode =
                        GetNode()->FindNode(pointNodeName);

                    SceneNode* cameraTargetNode =
                        GetNode()->FindNode(targetNodeName);

                    if (!cameraPointNode){
                        spdlog::warn(
                            "CraftingStation: missing {} camera point node: {}",
                            stageName,
                            pointNodeName
                        );
                    }

                    if (!cameraTargetNode){
                        spdlog::warn(
                            "CraftingStation: missing {} camera target node: {}",
                            stageName,
                            targetNodeName
                        );
                    }

                    if (!cameraPointNode || !cameraTargetNode){
                        spdlog::warn("CraftingStation: camera point or target missing. Using fallback camera transform.");

                        cameraNode->GlobalTransform().Position() =
                            stationCameraPosition;

                        cameraNode->GlobalTransform().Rotation() =
                            stationCameraRotation;

                        return;
                    }

                    glm::vec3 cameraPosition =
                        cameraPointNode->GlobalTransform().Position().Value();

                    glm::vec3 targetPosition =
                        cameraTargetNode->GlobalTransform().Position().Value();

                    spdlog::info(
                        "CraftingStation: using {} camera point global position: {} {} {}",
                        stageName,
                        cameraPosition.x,
                        cameraPosition.y,
                        cameraPosition.z
                    );

                    spdlog::info(
                        "CraftingStation: using {} camera target global position: {} {} {}",
                        stageName,
                        targetPosition.x,
                        targetPosition.y,
                        targetPosition.z
                    );

                    cameraNode->GlobalTransform().Position() =
                        cameraPosition;

                    cameraNode->GlobalTransform().Rotation() =
                        LookAtRotation(cameraPosition,targetPosition);
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

                    if (!ingredientsRootNode){
                        return;
                    }

                    if (ingredientsRootNode->EnabledSelf() == enabled){
                        return;
                    }

                    ingredientsRootNode->SetEnabled(enabled);
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

                    ingredientStageConfirmed = false;
                    SetLidInteractionEnabled(false);

                    OpenLid();

                    SetStationHitboxEnabled(false);

                    FocusCameraOnIngredientStage();

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

                    CloseLid();

                    SetLidInteractionEnabled(false);
                    SetStationHitboxEnabled(true);

                    EnablePlayer();
                    SetIngredientsEnabled(false);

                    if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                        cameraSettings->enabled = true;
                    }

                    isActive = false;

                    GetScene()->Input()->SetMouseLocked(false);

                    spdlog::info("CraftingStation: exited station view.");
                }
    };
}