#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"
#include "Texture.h"

#include "game_scripts/CameraSettings.h"
#include "game_scripts/PlayerController.h"
#include "game_scripts/PotionInventory.h"
#include "game_scripts/crafting/BottlingStage.h"
#include "game_scripts/crafting/Cauldron.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"
#include "game_scripts/crafting/CraftingInteractable.h"
#include "game_scripts/crafting/CraftingNodeUtils.h"
#include "game_scripts/crafting/DraggableCraftingItem.h"
#include "game_scripts/crafting/HeatingStage.h"

#include <text/Font.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>

#include <physics/Body.h>
#include <physics/System.h>
#include <TimeSystem.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

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

                bool lidInteractionEnabled = false;
                CraftingStage currentStage = CraftingStage::None;

                Cauldron* cauldron = nullptr;

                SceneNode* lidNode = nullptr;
                SceneNode* lidHitboxNode = nullptr;
                SceneNode* stationHitboxNode = nullptr;
                SceneNode* blowerHitboxNode = nullptr;
                SceneNode* doorHitboxNode = nullptr;
                SceneNode* valveHitboxNode = nullptr;

                bool lidHitboxIsChildOfLid = false;

                glm::vec3 lidClosedLocalPosition = glm::vec3(0.0f);
                glm::vec3 lidHitboxClosedLocalPosition = glm::vec3(0.0f);

                HeatingStage heatingStage;
                BottlingStage bottlingStage;

                bool blowerClickRequested = false;

                SceneNode* heatingUiRootNode = nullptr;
                UiText* heatingUiText = nullptr;

          public:
                float interactionRadius = 3.0f;

                std::string lidNodeName = "Lid";
                std::string lidHitboxNodeName = "LidHitbox";
                std::string stationHitboxNodeName = "StationHitbox";
                std::string blowerHitboxNodeName = "BlowerHitbox";
                std::string doorHitboxNodeName = "DoorHitbox";
                std::string valveHitboxNodeName = "ValveHitbox";

                std::string ingredientCameraPointNodeName = "StageOneStand";
                std::string ingredientCameraTargetNodeName = "StageOneLook";

                std::string heatingCameraPointNodeName = "StageTwoStand";
                std::string heatingCameraTargetNodeName = "StageTwoLook";

                std::string bottlingCameraPointNodeName = "LastStageStand";
                std::string bottlingCameraTargetNodeName = "LastStageLook";

                glm::vec3 lidOpenOffset = glm::vec3(-1.5f, 0.0f, 0.0f);

                void Awake(){
                    SceneNode* cauldronNode =
                        FindNodeRecursive(GetNode(), "Cauldron");

                    if (!cauldronNode){
                        return;
                    }

                    cauldron =
                        cauldronNode->GetObject<Cauldron>();

                    lidNode =
                        FindNodeRecursive(GetNode(), lidNodeName);

                    if (!lidNode){
                        return;
                    }

                    lidHitboxNode =
                        lidNode->FindNode(lidHitboxNodeName);

                    if (!lidHitboxNode){
                        lidHitboxNode =
                            FindNodeRecursive(GetNode(), lidHitboxNodeName);
                    }

                    if (lidHitboxNode){
                        lidHitboxIsChildOfLid =
                            IsNodeChildOf(lidHitboxNode,lidNode);
                    }

                    stationHitboxNode =
                        FindNodeRecursive(GetNode(), stationHitboxNodeName);

                    if (stationHitboxNode){
                        stationHitboxNode->SetEnabled(true);
                        SyncPhysicsBodyToNode(stationHitboxNode);
                    }

                    blowerHitboxNode =
                        FindNodeRecursive(GetNode(), blowerHitboxNodeName);

                    if (blowerHitboxNode){
                        blowerHitboxNode->SetEnabled(false);

                        if (auto* interactable = blowerHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(false);
                        }

                        SyncPhysicsBodyToNode(blowerHitboxNode);
                    }

                    doorHitboxNode =
                        FindNodeRecursive(GetNode(), doorHitboxNodeName);

                    if (doorHitboxNode){
                        doorHitboxNode->SetEnabled(false);

                        if (auto* interactable = doorHitboxNode->GetObject<CraftingInteractable>()){
                            interactable->SetInteractionEnabled(false);
                        }

                        SyncPhysicsBodyToNode(doorHitboxNode);
                    }

                    valveHitboxNode =
                        FindNodeRecursive(GetNode(), valveHitboxNodeName);

                    if (valveHitboxNode){
                        SetValveInteractionEnabled(false);
                        SyncPhysicsBodyToNode(valveHitboxNode);
                    }

                    bottlingStage.CacheNodes(GetNode());

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

                    CreateHeatingUi();
                    SetHeatingUiEnabled(false);
                }

                void Update(){
                    if (!GetScene() || !GetScene()->Input()){
                        return;
                    }

                    if (!isActive){
                        if (GetScene()->Input()->KeyDown(Key::G) || GetScene()->Input()->KeyDown(Key::E)){
                            EnterStation();
                        }

                        return;
                    }

                    if (currentStage == CraftingStage::Ingredients){
                        UpdateIngredientsStage();
                    }

                    if (currentStage == CraftingStage::Heating){
                        UpdateHeatingStage();
                    }

                    if (currentStage == CraftingStage::Bottling){
                        UpdateBottlingStage();
                    }

                    if (GetScene()->Input()->KeyDown(Key::Escape)){
                        ExitStation();
                    }
                }

                bool IsActive() const{
                    return isActive;
                }

                void ResetCraftingSession(){
                    currentStage = CraftingStage::None;
                    blowerClickRequested = false;

                    heatingStage.Reset();
                    bottlingStage.Reset();

                    if (cauldron){
                        cauldron->Clear();
                    }

                    ClearIngredientReceivers();
                    ResetDraggableIngredients();

                    SetLidInteractionEnabled(false);
                    SetDoorInteractionEnabled(false);
                    SetValveInteractionEnabled(false);
                    SetBlowerInteractionEnabled(false);
                    CloseLid();
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

                    if (currentStage == CraftingStage::Bottling){
                        if (CanUseValve()){
                            return ToMask(CraftingInteractionType::Valve);
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
                        blowerHitboxNode->IsEnabled() &&
                        heatingStage.CanClickBlower();
                }

                bool CanUseDoor() const{
                    return
                        isActive &&
                        currentStage == CraftingStage::Finished &&
                        doorHitboxNode &&
                        doorHitboxNode->IsEnabled();
                }

                bool CanUseValve() const{
                    return
                        isActive &&
                        currentStage == CraftingStage::Bottling &&
                        valveHitboxNode &&
                        valveHitboxNode->IsEnabled();
                }

                void OnLidClicked(){
                    if (!CanConfirmIngredientStageByLidClick()){
                        return;
                    }

                    CloseLid();

                    SetLidInteractionEnabled(false);

                    StartHeatingStage();

                    FocusCameraOnHeatingStage();
                }

                void OnBlowerClicked(){
                    if (!CanUseBlower()){
                        return;
                    }

                    blowerClickRequested = true;
                }

                void OnDoorClicked(){
                    if (!CanUseDoor()){
                        return;
                    }

                    StartBottlingStage();
                }

                void OnValveClicked(){
                    if (!CanUseValve()){
                        return;
                    }

                    bottlingStage.TryFillCurrentBottle();
                }

          private:
                void UpdateIngredientsStage(){
                    bool canConfirm =
                        cauldron && cauldron->CanConfirm();

                    SetLidInteractionEnabled(canConfirm);
                }

                void UpdateHeatingStage(){
                    float deltaTime = Time::Delta();

                    bool blowerClicked = blowerClickRequested;
                    blowerClickRequested = false;

                    bool finished =
                        heatingStage.Update(deltaTime, blowerClicked);

                    UpdateHeatingUi();

                    if (finished){
                        FinishHeatingStage();
                    }
                }

                void UpdateBottlingStage(){
                    float deltaTime = Time::Delta();

                    bool finished = bottlingStage.Update(deltaTime);

                    if (finished){
                        FinishBottlingStage();
                    }
                }

                void StartHeatingStage(){
                    currentStage = CraftingStage::Heating;

                    heatingStage.Start();

                    SetHeatingUiEnabled(true);
                    UpdateHeatingUi();

                    SetIngredientsEnabled(false);
                    SetDragInteractorEnabled(true);
                    SetDoorInteractionEnabled(false);
                    SetBlowerInteractionEnabled(true);
                }

                void FinishHeatingStage(){
                    currentStage = CraftingStage::Finished;

                    SetBlowerInteractionEnabled(false);
                    SetDragInteractorEnabled(true);
                    SetHeatingUiEnabled(false);

                    if (cauldron){
                        cauldron->SetQuality(heatingStage.GetQuality());
                    }

                    SetDoorInteractionEnabled(true);
                }

                void StartBottlingStage(){
                    currentStage = CraftingStage::Bottling;

                    SetDoorInteractionEnabled(false);
                    SetValveInteractionEnabled(true);
                    SetDragInteractorEnabled(true);

                    bottlingStage.Start();

                    FocusCameraOnBottlingStage();
                }

                void FinishBottlingStage(){
                    if (currentStage != CraftingStage::Bottling){
                        return;
                    }

                    SetValveInteractionEnabled(false);

                    SaveCraftedPotion();

                    ExitStation();
                }

                std::string GetPrimaryEffectId() const{
                    if (!cauldron){
                        return EffectId::None;
                    }

                    for (const auto& ingredient : cauldron->GetIngredients()){
                        if (ingredient.role == IngredientRole::MainEffect){
                            return ingredient.effectId;
                        }
                    }

                    return EffectId::None;
                }

                void SaveCraftedPotion(){
                    if (!cauldron || !bottlingStage.HasEnoughFilledBottles()){
                        return;
                    }

                    PotionInventory::SaveLastCraftedPotion(
                        cauldron->GetRecipeName(),
                        GetPrimaryEffectId(),
                        cauldron->GetQualityPercent(),
                        bottlingStage.GetFilledBottles()
                    );

                    if (PlayerController::Instance()){
                        PlayerController::Instance()->SetThrowingUnlocked(true);
                    }
                }

                void OpenLid(){
                    if (!lidNode){
                        return;
                    }

                    SetLidLocalPosition(
                        lidClosedLocalPosition + lidOpenOffset
                    );
                }

                void CloseLid(){
                    if (!lidNode){
                        return;
                    }

                    SetLidLocalPosition(lidClosedLocalPosition);
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
                }

                void SetDoorInteractionEnabled(bool enabled){
                    SetInteractionNodeEnabled(doorHitboxNode, enabled);
                }

                void SetValveInteractionEnabled(bool enabled){
                    SetInteractionNodeEnabled(valveHitboxNode, enabled);
                }

                void SetBlowerInteractionEnabled(bool enabled){
                    SetInteractionNodeEnabled(blowerHitboxNode, enabled);
                }

                void SetInteractionNodeEnabled(SceneNode* node, bool enabled){
                    if (!node){
                        return;
                    }

                    if (node->EnabledSelf() != enabled){
                        node->SetEnabled(enabled);
                    }

                    if (auto* interactable = node->GetObject<CraftingInteractable>()){
                        interactable->SetInteractionEnabled(enabled);
                    }

                    if (enabled){
                        SyncPhysicsBodyToNode(node);
                    }
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
                }

                void SetDragInteractorEnabled(bool enabled){
                    if (!GetScene()){
                        return;
                    }

                    SceneNode* dragInteractorNode =
                        FindNodeRecursive(GetNode(), "Crafting Drag Interactor");

                    if (!dragInteractorNode){
                        return;
                    }

                    if (dragInteractorNode->EnabledSelf() == enabled){
                        return;
                    }

                    dragInteractorNode->SetEnabled(enabled);
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

                glm::quat LookAtRotation(
                    const glm::vec3& cameraPosition,
                    const glm::vec3& targetPosition
                ){
                    glm::vec3 direction =
                        targetPosition - cameraPosition;

                    if (glm::length(direction) < 0.0001f){
                        return glm::quat(1.0f,0.0f,0.0f,0.0f);
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
                        ingredientCameraTargetNodeName
                    );
                }

                void FocusCameraOnHeatingStage(){
                    FocusCameraOnStage(
                        heatingCameraPointNodeName,
                        heatingCameraTargetNodeName
                    );
                }

                void FocusCameraOnBottlingStage(){
                    FocusCameraOnStage(
                        bottlingCameraPointNodeName,
                        bottlingCameraTargetNodeName
                    );
                }

                void FocusCameraOnStage(
                    const std::string& pointNodeName,
                    const std::string& targetNodeName
                ){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        return;
                    }

                    SceneNode* cameraPointNode =
                        FindNodeRecursive(GetNode(), pointNodeName);

                    SceneNode* cameraTargetNode =
                        FindNodeRecursive(GetNode(), targetNodeName);

                    if (!cameraPointNode || !cameraTargetNode){
                        return;
                    }

                    glm::vec3 cameraPosition =
                        cameraPointNode->GlobalTransform().Position().Value();

                    glm::vec3 targetPosition =
                        cameraTargetNode->GlobalTransform().Position().Value();

                    cameraNode->GlobalTransform().Position() =
                        cameraPosition;

                    cameraNode->GlobalTransform().Rotation() =
                        LookAtRotation(cameraPosition,targetPosition);
                }

                void ClearIngredientReceivers(){
                    std::vector<CraftingIngredientReceiver*> receivers;
                    CollectObjectsRecursive<CraftingIngredientReceiver>(GetNode(), receivers);

                    for (auto* receiver : receivers){
                        receiver->Clear();
                    }
                }

                void ResetDraggableIngredients(){
                    SceneNode* ingredientsRootNode = GetIngredientsRootNode();

                    if (!ingredientsRootNode){
                        return;
                    }

                    std::vector<DraggableCraftingItem*> items;
                    CollectObjectsRecursive<DraggableCraftingItem>(ingredientsRootNode, items);

                    for (auto* item : items){
                        item->ResetForNewSession();
                    }
                }

                SceneNode* GetPlayerNode(){
                    return GetScene()->FindNode("Player");
                }

                SceneNode* GetCameraNode(){
                    return GetScene()->FindNode("Camera Node");
                }

                SceneNode* GetIngredientsRootNode(){
                    return FindNodeRecursive(GetNode(), "Crafting Ingredients");
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
                        stationHitboxNode
                            ? stationHitboxNode->GlobalTransform().Position().Value()
                            : GetNode()->GlobalTransform().Position().Value();

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


                void CreateHeatingUi(){
                    if (heatingUiRootNode || !GetScene()){
                        return;
                    }

                    TextureParams fontTextureParams = {
                        .channels = TextureChannels::RGB,
                        .colorSpace = TextureColor::Linear,
                        .format = TextureFormat::Ubyte,
                        .wrapU = TextureWrap::Clamp,
                        .wrapV = TextureWrap::Clamp,
                        .minFilter = TextureFilter::Linear,
                        .magFilter = TextureFilter::Linear
                    };

                    Texture2D* fontTexture =
                        GetScene()->Resources()->Get<Texture2D>(
                            "./res/fonts/OpenSans-Regular/OpenSans-Regular.png",
                            fontTextureParams
                        );

                    Font* font =
                        GetScene()->Resources()->Get<Font>(
                            "./res/fonts/OpenSans-Regular/OpenSans-Regular.json",
                            fontTexture
                        );

                    heatingUiRootNode =
                        GetScene()->CreateNode("Crafting Heating UI Root");

                    heatingUiRootNode->AddObject<UiLayout>(
                        glm::ivec2(280, 110),
                        glm::ivec2(-20, 20),
                        20,
                        AnchorPoint::TopRight
                    );

                    heatingUiRootNode->AddObject<UiVisual>(
                        glm::vec4(0.0f, 0.0f, 0.0f, 0.55f)
                    );

                    SceneNode* textNode =
                        GetScene()->CreateNode(
                            heatingUiRootNode,
                            "Crafting Heating UI Text"
                        );

                    textNode->AddObject<UiLayout>(
                        glm::ivec2(260, 90),
                        glm::ivec2(12, 8),
                        21,
                        AnchorPoint::TopLeft
                    );

                    heatingUiText =
                        textNode->AddObject<UiText>("", font);

                    heatingUiText->fontSize = 22.0f;
                    heatingUiText->color = glm::vec4(1.0f, 0.93f, 0.72f, 1.0f);
                    heatingUiText->alignment = TextAlignment::Left;
                }

                void SetHeatingUiEnabled(bool enabled){
                    if (!heatingUiRootNode){
                        return;
                    }

                    if (heatingUiRootNode->EnabledSelf() == enabled){
                        return;
                    }

                    heatingUiRootNode->SetEnabled(enabled);
                }

                void UpdateHeatingUi(){
                    if (!heatingUiText){
                        return;
                    }

                    std::ostringstream text;
                    text << std::fixed << std::setprecision(1);
                    text << "Heating\n";
                    text << "Target: " << heatingStage.tempMin << " - " << heatingStage.tempMax << " C\n";
                    text << "Temp: " << heatingStage.GetTemperature() << " C\n";
                    text << "Quality: " << heatingStage.GetQualityPercent() << "%";

                    heatingUiText->text = text.str();
                }

                void EnterStation(){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        return;
                    }

                    Camera* camera = cameraNode->GetObject<Camera>();

                    if (!camera){
                        return;
                    }

                    if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                        cameraSettings->SetEnabled(false);
                    }

                    ResetCraftingSession();
                    currentStage = CraftingStage::Ingredients;

                    SetDragInteractorEnabled(true);
                    SetLidInteractionEnabled(false);
                    SetDoorInteractionEnabled(false);
                    SetValveInteractionEnabled(false);
                    SetBlowerInteractionEnabled(false);

                    OpenLid();

                    SetStationHitboxEnabled(false);

                    FocusCameraOnIngredientStage();

                    camera->SetAsMainCamera();

                    DisablePlayer();
                    SetIngredientsEnabled(true);

                    isActive = true;

                    GetScene()->Input()->SetMouseLocked(false);
                }

                void ExitStation(){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        return;
                    }

                    ResetCraftingSession();

                    SetBlowerInteractionEnabled(false);
                    SetDoorInteractionEnabled(false);
                    SetValveInteractionEnabled(false);
                    SetHeatingUiEnabled(false);
                    bottlingStage.Stop();

                    SetDragInteractorEnabled(true);

                    CloseLid();

                    SetLidInteractionEnabled(false);
                    SetStationHitboxEnabled(true);

                    EnablePlayer();
                    SetIngredientsEnabled(false);

                    if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                        cameraSettings->SetEnabled(true);
                    }

                    isActive = false;

                    GetScene()->Input()->SetMouseLocked(false);
                }
    };
}
