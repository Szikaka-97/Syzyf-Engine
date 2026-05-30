#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"

#include "game_scripts/CameraSettings.h"
#include "game_scripts/crafting/Cauldron.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"
#include "game_scripts/crafting/CraftingInteractable.h"
#include "game_scripts/crafting/HeatingStage.h"

#include <physics/Body.h>
#include <physics/System.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <spdlog/spdlog.h>

#include <string>

namespace Crafting{
    class CraftingStation : public GameObject{
          private:
                enum class CraftingStage{
                    None,
                    Ingredients,
                    Heating,
                    Finished,
                    Bottling
                };

                bool isActive = false;
                bool playerWasEnabled = true;

                bool ingredientStageConfirmed = false;
                bool lidInteractionEnabled = false;
                bool loggedIngredientsReady = false;

                CraftingStage currentStage = CraftingStage::None;

                Cauldron* cauldron = nullptr;

                SceneNode* lidNode = nullptr;
                SceneNode* lidHitboxNode = nullptr;
                SceneNode* stationHitboxNode = nullptr;
                SceneNode* blowerHitboxNode = nullptr;
                SceneNode* doorHitboxNode = nullptr;

                bool lidHitboxIsChildOfLid = false;

                glm::vec3 lidClosedLocalPosition = glm::vec3(0.0f);
                glm::vec3 lidHitboxClosedLocalPosition = glm::vec3(0.0f);

                HeatingStage heatingStage;

                bool blowerClickRequested = false;

          public:
                float interactionRadius = 3.0f;

                std::string lidNodeName = "Lid";
                std::string lidHitboxNodeName = "LidHitbox";
                std::string stationHitboxNodeName = "StationHitbox";
                std::string blowerHitboxNodeName = "BlowerHitbox";
                std::string doorNodeName = "Door";
                std::string doorHitboxNodeName = "DoorHitbox";

                std::string ingredientCameraPointNodeName = "IngredientCameraPoint";
                std::string ingredientCameraTargetNodeName = "IngredientCameraTarget";

                std::string heatingCameraPointNodeName = "HeatingCameraPoint";
                std::string heatingCameraTargetNodeName = "HeatingCameraTarget";

                std::string bottlingCameraPointNodeName = "BottlingCameraPoint";
                std::string bottlingCameraTargetNodeName = "BottlingCameraTarget";

                glm::vec3 lidOpenOffset = glm::vec3(-1.5f, 0.0f, 0.0f);

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

                    blowerHitboxNode =
                        GetNode()->FindNode(blowerHitboxNodeName);

                    if (!blowerHitboxNode){
                        blowerHitboxNode =
                            FindNodeRecursive(GetNode(), blowerHitboxNodeName);
                    }

                    if (!blowerHitboxNode){
                        spdlog::warn("CraftingStation: BlowerHitbox node not found.");
                    }else{
                        blowerHitboxNode->SetEnabled(false);

                        if (auto* interactable = blowerHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(false);
                        }

                        SyncPhysicsBodyToNode(blowerHitboxNode);

                        glm::vec3 blowerPosition =
                            blowerHitboxNode->GlobalTransform().Position().Value();

                        spdlog::info(
                            "CraftingStation: BlowerHitbox found at {} {} {}.",
                            blowerPosition.x,
                            blowerPosition.y,
                            blowerPosition.z
                        );
                    }

                    doorHitboxNode =
                        GetNode()->FindNode(doorHitboxNodeName);

                    if (!doorHitboxNode){
                        doorHitboxNode =
                            FindNodeRecursive(GetNode(), doorHitboxNodeName);
                    }

                    if (!doorHitboxNode){
                        spdlog::warn("CraftingStation: DoorHitbox node not found.");
                    }else{
                        doorHitboxNode->SetEnabled(false);

                        if (auto* interactable = doorHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(false);
                        }

                        SyncPhysicsBodyToNode(doorHitboxNode);

                        glm::vec3 doorPosition =
                            doorHitboxNode->GlobalTransform().Position().Value();

                        spdlog::info(
                            "CraftingStation: DoorHitbox found at {} {} {}.",
                            doorPosition.x,
                            doorPosition.y,
                            doorPosition.z
                        );
                    }

                    lidClosedLocalPosition =
                        lidNode->LocalTransform().Position().Value();

                    if (lidHitboxNode){
                        lidHitboxClosedLocalPosition =
                            lidHitboxNode->LocalTransform().Position().Value();
                    }

                    CloseLid();

                    currentStage = CraftingStage::None;

                    SetLidInteractionEnabled(false);
                    SetDoorInteractionEnabled(false);
                    SetStationHitboxEnabled(true);
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

                    if (currentStage == CraftingStage::Ingredients){
                        UpdateIngredientsStage();
                    }

                    if (currentStage == CraftingStage::Heating){
                        UpdateHeatingStage();
                    }

                    if (GetScene()->Input()->KeyDown(Key::Escape)){
                        ExitStation();
                    }
                }

                bool IsActive() const{
                    return isActive;
                }

                CraftingInteractionMask GetActiveInteractionMask() const{
                    if (!isActive){
                        return ToMask(CraftingInteractionType::None);
                    }

                    if (currentStage == CraftingStage::Ingredients){
                        CraftingInteractionMask mask = ToMask(CraftingInteractionType::Ingredient);

                        if (CanConfirmIngredientStageByLidClick()){
                            mask = mask | CraftingInteractionType::Lid;
                        }

                        return mask;
                    }

                    if (currentStage == CraftingStage::Heating){
                        return ToMask(CraftingInteractionType::Blower);
                    }

                    if (currentStage == CraftingStage::Finished){
                        if (CanUseDoor()){
                            return ToMask(CraftingInteractionType::Door);
                        }

                        return ToMask(CraftingInteractionType::None);
                    }

                    return ToMask(CraftingInteractionType::None);
                }

                bool CanConfirmIngredientStageByLidClick() const{
                    return
                        isActive &&
                        currentStage == CraftingStage::Ingredients &&
                        lidInteractionEnabled &&
                        lidHitboxNode &&
                        cauldron &&
                        cauldron->CanConfirm();
                }

                bool CanUseBlower() const{
                    return
                        isActive &&
                        currentStage == CraftingStage::Heating &&
                        blowerHitboxNode &&
                        blowerHitboxNode->IsEnabled();
                }

                bool CanUseDoor() const{
                    return
                        isActive &&
                        currentStage == CraftingStage::Finished &&
                        doorHitboxNode &&
                        doorHitboxNode->IsEnabled();
                }

                void OnLidClicked(){
                    if (!CanConfirmIngredientStageByLidClick()){
                        spdlog::warn("CraftingStation: Lid click ignored.");
                        return;
                    }

                    ingredientStageConfirmed = true;

                    CloseLid();

                    SetLidInteractionEnabled(false);

                    StartHeatingStage();

                    FocusCameraOnHeatingStage();

                    spdlog::info("CraftingStation: ingredient stage confirmed by Lid click.");
                }

                void OnBlowerClicked(){
                    if (!CanUseBlower()){
                        spdlog::warn("CraftingStation: Blower click ignored.");
                        return;
                    }

                    blowerClickRequested = true;

                    spdlog::info("CraftingStation: Blower click accepted.");
                }

                void OnDoorClicked(){
                    if (!CanUseDoor()){
                        spdlog::warn("CraftingStation: Door click ignored.");
                        return;
                    }

                    StartBottlingStage();

                    spdlog::info("CraftingStation: Door clicked. Bottling stage started.");
                }

          private:
                void UpdateIngredientsStage(){
                    bool canConfirm =
                        cauldron && cauldron->CanConfirm();

                    SetLidInteractionEnabled(canConfirm);

                    if (canConfirm && !loggedIngredientsReady){
                        loggedIngredientsReady = true;

                        spdlog::info(
                            "CraftingStation: ingredients ready. Click Lid to start heating."
                        );
                    }

                    if (!canConfirm){
                        loggedIngredientsReady = false;
                    }
                }

                void UpdateHeatingStage(){
                    float deltaTime = 1.0f / 60.0f;

                    bool blowerClicked = blowerClickRequested;
                    blowerClickRequested = false;

                    bool finished =
                        heatingStage.Update(deltaTime, blowerClicked);

                    if (finished){
                        FinishHeatingStage();
                    }
                }

                void StartHeatingStage(){
                    currentStage = CraftingStage::Heating;

                    heatingStage.Start();

                    SetIngredientsEnabled(false);
                    SetDragInteractorEnabled(true);
                    SetDoorInteractionEnabled(false);

                    if (blowerHitboxNode){
                        blowerHitboxNode->SetEnabled(true);

                        if (auto* interactable = blowerHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(true);
                        }

                        SyncPhysicsBodyToNode(blowerHitboxNode);
                    }

                    spdlog::info("CraftingStation: heating stage started.");
                }

                void FinishHeatingStage(){
                    currentStage = CraftingStage::Finished;

                    if (blowerHitboxNode){
                        blowerHitboxNode->SetEnabled(false);

                        if (auto* interactable = blowerHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(false);
                        }
                    }

                    SetDragInteractorEnabled(true);

                    if (cauldron){
                        cauldron->SetQuality(heatingStage.GetQuality());
                    }

                    SetDoorInteractionEnabled(true);

                    spdlog::info(
                        "CraftingStation: heating finished. Quality: {} Temperature: {}. Click Door to continue.",
                        heatingStage.GetQuality(),
                        heatingStage.GetTemperature()
                    );
                }

                void StartBottlingStage(){
                    currentStage = CraftingStage::Bottling;

                    SetDoorInteractionEnabled(false);
                    SetDragInteractorEnabled(true);

                    FocusCameraOnBottlingStage();

                    spdlog::info("CraftingStation: bottling stage active.");
                }

                void OpenLid(){
                    if (!lidNode){
                        return;
                    }

                    SetLidLocalPosition(
                        lidClosedLocalPosition + lidOpenOffset
                    );

                    spdlog::info("CraftingStation: Lid opened.");
                }

                void CloseLid(){
                    if (!lidNode){
                        return;
                    }

                    SetLidLocalPosition(lidClosedLocalPosition);

                    spdlog::info("CraftingStation: Lid closed.");
                }

                void SetLidInteractionEnabled(bool enabled){
                    if (lidInteractionEnabled == enabled){
                        if (lidHitboxNode && lidHitboxNode->EnabledSelf() != enabled){
                            lidHitboxNode->SetEnabled(enabled);

                            if (auto* interactable = lidHitboxNode->GetObject<CraftingInteractable>()){
                                interactable->SetInteractionEnabled(enabled);
                            }

                            if (enabled){
                                SyncPhysicsBodyToNode(lidHitboxNode);
                            }
                        }

                        return;
                    }

                    lidInteractionEnabled = enabled;

                    if (lidHitboxNode){
                        lidHitboxNode->SetEnabled(enabled);

                        if (auto* interactable = lidHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(enabled);
                        }

                        if (enabled){
                            SyncPhysicsBodyToNode(lidHitboxNode);
                        }
                    }

                    spdlog::info(
                        "CraftingStation: Lid interaction {}.",
                        enabled ? "enabled" : "disabled"
                    );
                }

                void SetDoorInteractionEnabled(bool enabled){
                    if (!doorHitboxNode){
                        return;
                    }

                    if (doorHitboxNode->EnabledSelf() != enabled){
                        doorHitboxNode->SetEnabled(enabled);
                    }

                    if (auto* interactable = doorHitboxNode->GetObject<CraftingInteractable>()){
                        interactable->SetInteractionEnabled(enabled);
                    }

                    if (enabled){
                        SyncPhysicsBodyToNode(doorHitboxNode);
                    }

                    spdlog::info(
                        "CraftingStation: Door interaction {}.",
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

                void SetDragInteractorEnabled(bool enabled){
                    if (!GetScene()){
                        return;
                    }

                    SceneNode* dragInteractorNode =
                        GetScene()->FindNode("Crafting Root/Root/Crafting Drag Interactor");

                    if (!dragInteractorNode){
                        return;
                    }

                    if (dragInteractorNode->EnabledSelf() == enabled){
                        return;
                    }

                    dragInteractorNode->SetEnabled(enabled);

                    spdlog::info(
                        "CraftingStation: drag interactor {}.",
                        enabled ? "enabled" : "disabled"
                    );
                }

                void SetLidLocalPosition(const glm::vec3& localPosition){

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

                SceneNode* FindNodeRecursive(SceneNode* rootNode, const std::string& targetName){
                    if (!rootNode){
                        return nullptr;
                    }

                    if (rootNode->GetName() == targetName){
                        return rootNode;
                    }

                    for (SceneNode* child : rootNode->GetChildren()){
                        if (SceneNode* foundNode = FindNodeRecursive(child, targetName)){
                            return foundNode;
                        }
                    }

                    return nullptr;
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
                    if (!node){
                        return;
                    }

                    auto* body = node->GetObject<Physics::Body>();

                    if (!body){
                        return;
                    }

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

                glm::quat LookAtRotation(
                    const glm::vec3& cameraPosition,
                    const glm::vec3& targetPosition
                ){
                    glm::vec3 direction =
                        targetPosition - cameraPosition;

                    if (glm::length(direction) < 0.0001f){
                        spdlog::warn(
                            "CraftingStation: camera point and target are too close. Using fallback rotation."
                        );

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

                void FocusCameraOnBottlingStage(){
                    FocusCameraOnStage(
                        bottlingCameraPointNodeName,
                        bottlingCameraTargetNodeName,
                        "bottling"
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
                        spdlog::warn(
                            "CraftingStation: camera point or target missing. Using fallback camera transform."
                        );

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

                    currentStage = CraftingStage::Ingredients;
                    ingredientStageConfirmed = false;
                    loggedIngredientsReady = false;
                    blowerClickRequested = false;

                    SetDragInteractorEnabled(true);
                    SetLidInteractionEnabled(false);
                    SetDoorInteractionEnabled(false);

                    if (blowerHitboxNode){
                        blowerHitboxNode->SetEnabled(false);

                        if (auto* interactable = blowerHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(false);
                        }
                    }

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

                    currentStage = CraftingStage::None;
                    blowerClickRequested = false;

                    if (blowerHitboxNode){
                        blowerHitboxNode->SetEnabled(false);

                        if (auto* interactable = blowerHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(false);
                        }
                    }

                    SetDoorInteractionEnabled(false);

                    SetDragInteractorEnabled(true);

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