#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"
#include "Graphics.h"
#include "Texture.h"
#include "Material.h"
#include "MeshRenderer.h"
#include "Shader.h"

#include "game_scripts/CameraSettings.h"
#include "game_scripts/PlayerController.h"
#include "../../include/game_scripts/PotionInventory.h"
#include "game_scripts/crafting/BottlingStage.h"
#include "game_scripts/crafting/Cauldron.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"
#include "game_scripts/crafting/CraftingInteractable.h"
#include "game_scripts/crafting/CraftingNodeUtils.h"
#include "game_scripts/crafting/DraggableCraftingItem.h"
#include "game_scripts/crafting/HeatingStage.h"

#include <text/Font.h>
#include <text/Text3D.h>
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

#include <array>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>

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

                enum class TutorialFocus{
                    None,
                    IngredientInventory,
                    IngredientCauldron,
                    IngredientConfirm,
                    HeatingBlower,
                    HeatingDoor,
                    BottlingValve,
                    BottlingDone
                };

                bool isActive = false;
                bool playerWasEnabled = true;

                bool savedCameraTransformValid = false;
                glm::vec3 savedCameraPosition = glm::vec3(0.0f);
                glm::quat savedCameraRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                float savedCameraFov = 60.0f;
                float savedCameraAspectRatio = 16.0f / 9.0f;
                float savedCameraNearPlane = 0.1f;
                float savedCameraFarPlane = 200.0f;

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
                SceneNode* dungeonPromptUiNode = nullptr;
                UiText* dungeonPromptText = nullptr;
                bool dungeonPromptMessageVisible = false;
                float dungeonPromptShowUntilTime = 0.0f;

                bool craftingTutorialVisible = false;
                TutorialFocus activeTutorialFocus = TutorialFocus::None;
                SceneNode* tutorialTextPanelNode = nullptr;
                UiLayout* tutorialTextPanelLayout = nullptr;
                UiText* tutorialText = nullptr;
                std::array<SceneNode*, 4> tutorialDimPanelNodes = {};
                std::array<UiLayout*, 4> tutorialDimPanelLayouts = {};
                std::array<SceneNode*, 4> tutorialBorderPanelNodes = {};
                std::array<UiLayout*, 4> tutorialBorderPanelLayouts = {};

                SceneNode* gameplayHudNode = nullptr;
                bool gameplayHudVisibilitySaved = false;
                bool gameplayHudWasEnabled = true;

                SceneNode* bellowNode = nullptr;
                SceneNode* bladeNode = nullptr;
                SceneNode* stageTwoDeviceNode = nullptr;
                SceneNode* stageTwoColorNode = nullptr;
                Material* stageTwoTemperatureArcMaterial = nullptr;
                SceneNode* qualityMeterNode = nullptr;

                glm::vec3 bellowDefaultScale = glm::vec3(1.0f);
                glm::quat bladeDefaultLocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                glm::quat stageTwoColorDefaultLocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                float bellowAnimationAmount = 0.0f;

                Text3D* stageTwoQualityText = nullptr;
                Text3D* stageTwoTemperatureText = nullptr;
                std::array<SceneNode*, 5> qualityStarNodes = {};

                SceneNode* lastStagePotionPanelNode = nullptr;
                Text3D* lastStagePotionInfoText = nullptr;
                glm::vec3 lastStagePotionPanelDefaultPosition = glm::vec3(0.0f);
                glm::vec3 lastStagePotionPanelHiddenPosition = glm::vec3(0.0f);
                std::array<SceneNode*, 5> lastStagePotionStarNodes = {};
                std::array<glm::vec3, 5> lastStagePotionStarDefaultPositions = {};
                std::array<glm::vec3, 5> lastStagePotionStarHiddenPositions = {};
                int lastStagePotionVisibleStars = 0;
                float lastStagePotionPanelSlide = 0.0f;
                bool lastStagePotionPanelVisible = false;
                bool lastStagePotionTextVisible = false;
                std::string lastStagePotionResultText;
                bool craftedPotionDataValid = false;
                CraftedPotionData craftedPotionData;

                SceneNode* stageOneUiNode = nullptr;
                SceneNode* stageTwoUiNode = nullptr;
                SceneNode* lastStageUiNode = nullptr;

                SceneNode* stageOneCameraLightNode = nullptr;
                SceneNode* stageTwoCameraLightNode = nullptr;
                SceneNode* lastStageCameraLightNode = nullptr;

                int inventoryPage = 0;

                std::vector<PotionInventory::IngredientInventoryEntry> playerInventory;
                std::array<Text3D*, 4> inventorySlotTexts = {};
                std::array<Text3D*, 4> cauldronSlotTexts = {};

          public:
                float interactionRadius = 3.0f;
                bool enterStationOnFirstUpdate = false;

                float thermometerMinTemperature = 0.0f;
                float thermometerNeedleMinAngleDegrees = 90.0f;
                float thermometerNeedleMaxAngleDegrees = -90.0f;
                bool thermometerUseHeatingRangeAsScale = true;
                float thermometerFallbackMaxTemperature = 100.0f;
                bool thermometerArcStartsAtTargetMinimum = false;
                float thermometerOverheatVisualRangeMultiplier = 1.0f;

                float thermometerArcLocalRadius = 0.6328577f;
                float thermometerArcInnerRadius01 = 0.0f;
                float thermometerArcOuterRadius01 = 0.86f;
                float thermometerArcEdgeSoftness01 = 0.015f;
                float thermometerArcAngleOffsetDegrees = 0.0f;
                float thermometerArcStartDegrees = -90.0f;
                float thermometerArcEndDegrees = 90.0f;
                glm::vec4 thermometerSafeRangeColor = glm::vec4(0.17f, 0.48f, 0.12f, 1.0f);
                glm::vec4 thermometerOverheatRangeColor = glm::vec4(0.02f, 0.02f, 0.018f, 1.0f);
                glm::vec4 thermometerBaseFillColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

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

                std::string ingredientCameraNodeName = "StageOneCamera";
                std::string heatingCameraNodeName = "StageTwoCamera";
                std::string bottlingCameraNodeName = "LastStageCamera";

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
                        FindNodeRecursive(GetNode(), "Knob_One.001");

                    if (!valveHitboxNode){
                        valveHitboxNode =
                            FindNodeRecursive(GetNode(), "Knob_One");
                    }

                    if (!valveHitboxNode){
                        valveHitboxNode =
                            FindNodeRecursive(GetNode(), valveHitboxNodeName);
                    }

                    if (valveHitboxNode){
                        SetValveInteractionEnabled(false);
                        SyncPhysicsBodyToNode(valveHitboxNode);
                    }

                    bottlingStage.CacheNodes(GetNode());

                    stageOneUiNode =
                        FindNodeRecursive(GetNode(), "StageOneUI");

                    stageTwoUiNode =
                        FindNodeRecursive(GetNode(), "StageTwoUI");

                    lastStageUiNode =
                        FindNodeRecursive(GetNode(), "LastStageUI");

                    stageOneCameraLightNode =
                        FindNodeRecursive(GetNode(), "StageOneCameraLight");

                    stageTwoCameraLightNode =
                        FindNodeRecursive(GetNode(), "StageTwoCameraLight");

                    lastStageCameraLightNode =
                        FindNodeRecursive(GetNode(), "LastStageCameraLight");

                    SceneNode* stageTwoSearchRoot =
                        stageTwoUiNode ? stageTwoUiNode : GetNode();

                    bellowNode =
                        FindNodeRecursive(stageTwoSearchRoot, "bellow");

                    bladeNode =
                        FindNodeRecursive(stageTwoSearchRoot, "blade");

                    stageTwoDeviceNode =
                        FindNodeRecursive(stageTwoSearchRoot, "device");

                    stageTwoColorNode =
                        FindNodeRecursive(stageTwoSearchRoot, "color");

                    qualityMeterNode =
                        FindNodeRecursive(stageTwoSearchRoot, "quality_metter.003");

                    if (!qualityMeterNode){
                        qualityMeterNode =
                            FindNodeRecursive(stageTwoSearchRoot, "quality_metter");
                    }

                    qualityStarNodes[0] =
                        FindNodeRecursive(GetNode(), "Star.001");

                    qualityStarNodes[1] =
                        FindNodeRecursive(GetNode(), "Star.002");

                    qualityStarNodes[2] =
                        FindNodeRecursive(GetNode(), "Star.003");

                    qualityStarNodes[3] =
                        FindNodeRecursive(GetNode(), "Star.004");

                    qualityStarNodes[4] =
                        FindNodeRecursive(GetNode(), "Star.005");

                    lastStagePotionPanelNode =
                        FindNodeRecursive(GetNode(), "quality_metter.001");

                    if (lastStagePotionPanelNode){
                        lastStagePotionPanelDefaultPosition =
                            lastStagePotionPanelNode->LocalTransform().Position().Value();

                        lastStagePotionPanelHiddenPosition =
                            lastStagePotionPanelDefaultPosition + glm::vec3(0.0f, -0.55f, 0.0f);

                        lastStagePotionPanelNode->LocalTransform().Position() =
                            lastStagePotionPanelHiddenPosition;

                        lastStagePotionPanelNode->SetEnabled(false);
                    }

                    lastStagePotionStarNodes[0] =
                        FindNodeRecursive(GetNode(), "Star.006");

                    lastStagePotionStarNodes[1] =
                        FindNodeRecursive(GetNode(), "Star.007");

                    lastStagePotionStarNodes[2] =
                        FindNodeRecursive(GetNode(), "Star.008");

                    lastStagePotionStarNodes[3] =
                        FindNodeRecursive(GetNode(), "Star.009");

                    lastStagePotionStarNodes[4] =
                        FindNodeRecursive(GetNode(), "Star.010");

                    for (int i = 0; i < 5; i++){
                        if (!lastStagePotionStarNodes[i]){
                            continue;
                        }

                        lastStagePotionStarDefaultPositions[i] =
                            lastStagePotionStarNodes[i]->LocalTransform().Position().Value();

                        lastStagePotionStarHiddenPositions[i] =
                            lastStagePotionStarDefaultPositions[i] + glm::vec3(0.0f, -0.55f, 0.0f);

                        lastStagePotionStarNodes[i]->LocalTransform().Position() =
                            lastStagePotionStarHiddenPositions[i];

                        lastStagePotionStarNodes[i]->SetEnabled(false);
                    }

                    if (bellowNode){
                        bellowDefaultScale =
                            bellowNode->LocalTransform().Scale().Value();
                    }

                    if (bladeNode){
                        bladeDefaultLocalRotation =
                            bladeNode->LocalTransform().Rotation().Value();
                    }

                    if (stageTwoColorNode){
                        stageTwoColorDefaultLocalRotation =
                            stageTwoColorNode->LocalTransform().Rotation().Value();
                    }

                    CreateStageTwoTemperatureArcMaterial();

                    RefreshPlayerInventoryFromPersistent();

                    CreateStageOneModelTexts();
                    CreateStageTwoModelTexts();
                    CreateLastStageModelTexts();
                    UpdateInventorySlotTexts();
                    UpdateCauldronSlotTexts();
                    UpdateHeatingModelUi();

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
                    CreateDungeonPromptUi();
                    HideDungeonPromptMessage();
                    CreateCraftingTutorialUi();
                    HideCraftingTutorial();

                    if (stageOneUiNode){
                        stageOneUiNode->SetEnabled(false);
                    }

                    if (stageTwoUiNode){
                        stageTwoUiNode->SetEnabled(false);
                    }

                    if (lastStageUiNode){
                        lastStageUiNode->SetEnabled(false);
                    }

                    if (stageOneCameraLightNode){
                        stageOneCameraLightNode->SetEnabled(false);
                    }

                    if (stageTwoCameraLightNode){
                        stageTwoCameraLightNode->SetEnabled(false);
                    }

                    if (lastStageCameraLightNode){
                        lastStageCameraLightNode->SetEnabled(false);
                    }
                }

                void Update(){
                    if (!GetScene() || !GetScene()->Input()){
                        return;
                    }

                    UpdateDungeonPromptMessage();

                    if (!isActive){
                        if (enterStationOnFirstUpdate){
                            enterStationOnFirstUpdate = false;
                            EnterStation();
                            return;
                        }

                        if (GetScene()->Input()->KeyDown(Key::F) && IsPlayerNear()){
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

                    UpdateCraftingTutorial();

                    if (GetScene()->Input()->KeyDown(Key::Escape)){
                        ExitStation();
                    }
                }

                bool IsActive() const{
                    return isActive;
                }

                void ResetCraftingSession(){
                    bool shouldRefundIngredientDraft =
                        currentStage == CraftingStage::Ingredients;

                    currentStage = CraftingStage::None;
                    blowerClickRequested = false;
                    craftedPotionDataValid = false;
                    lastStagePotionPanelVisible = false;
                    lastStagePotionTextVisible = false;
                    lastStagePotionResultText.clear();
                    lastStagePotionPanelSlide = 0.0f;
                    HideDungeonPromptMessage();
                    HideCraftingTutorial();

                    if (lastStagePotionPanelNode){
                        lastStagePotionPanelNode->SetEnabled(false);
                        lastStagePotionPanelNode->LocalTransform().Position() =
                            lastStagePotionPanelHiddenPosition;
                    }

                    if (lastStagePotionInfoText){
                        lastStagePotionInfoText->SetText("");
                    }

                    lastStagePotionVisibleStars = 0;

                    for (int i = 0; i < 5; i++){
                        if (!lastStagePotionStarNodes[i]){
                            continue;
                        }

                        lastStagePotionStarNodes[i]->SetEnabled(false);
                        lastStagePotionStarNodes[i]->LocalTransform().Position() =
                            lastStagePotionStarHiddenPositions[i];
                    }

                    if (stageOneUiNode){
                        stageOneUiNode->SetEnabled(false);
                    }

                    if (stageTwoUiNode){
                        stageTwoUiNode->SetEnabled(false);
                    }

                    if (lastStageUiNode){
                        lastStageUiNode->SetEnabled(false);
                    }

                    if (stageOneCameraLightNode){
                        stageOneCameraLightNode->SetEnabled(false);
                    }

                    if (stageTwoCameraLightNode){
                        stageTwoCameraLightNode->SetEnabled(false);
                    }

                    if (lastStageCameraLightNode){
                        lastStageCameraLightNode->SetEnabled(false);
                    }

                    heatingStage.Reset();
                    bottlingStage.Reset();

                    if (cauldron){
                        if (shouldRefundIngredientDraft){
                            for (const Crafting::IngredientData& ingredient : cauldron->GetIngredients()){
                                PotionInventory::RefundIngredient(ingredient.inventoryKey,1);
                            }
                        }

                        cauldron->Clear();
                    }

                    UpdateCauldronSlotTexts();

                    ClearIngredientReceivers();
                    ResetDraggableIngredients();
                    RefreshPlayerInventoryFromPersistent();
                    UpdateInventorySlotTexts();
                    UpdateVisibleInventoryItems();

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

                    CraftingInteractionMask uiMask =
                        CraftingInteractionType::UiBack |
                        CraftingInteractionType::UiInfo |
                        CraftingInteractionType::UiNext;

                    if (currentStage == CraftingStage::Ingredients){
                        CraftingInteractionMask mask =
                            ToMask(CraftingInteractionType::Ingredient) |
                            uiMask |
                            CraftingInteractionType::InventoryNextPage |
                            CraftingInteractionType::InventoryPreviousPage;

                        if (CanConfirmIngredientStageByLidClick()){
                            mask = mask | CraftingInteractionType::Lid;
                        }

                        return mask;
                    }

                    if (currentStage == CraftingStage::Heating){
                        return ToMask(CraftingInteractionType::Blower) | uiMask;
                    }

                    if (currentStage == CraftingStage::Finished){
                        CraftingInteractionMask mask = uiMask;

                        if (CanUseDoor()){
                            mask = mask | CraftingInteractionType::Door;
                        }

                        return mask;
                    }

                    if (currentStage == CraftingStage::Bottling){
                        CraftingInteractionMask mask = uiMask;

                        if (CanUseValve()){
                            mask = mask | CraftingInteractionType::Valve;
                        }

                        return mask;
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

                void ConfirmIngredientStage(){
                    if (!cauldron || !cauldron->CanConfirm()){
                        spdlog::info("Crafting UI Next: add at least one main ingredient before brewing.");
                        return;
                    }

                    CloseLid();

                    SetLidInteractionEnabled(false);

                    StartHeatingStage();

                    FocusCameraOnHeatingStage();
                }

                void OnLidClicked(){
                    if (!CanConfirmIngredientStageByLidClick()){
                        return;
                    }

                    ConfirmIngredientStage();
                }

                void OnBlowerClicked(){
                    if (!CanUseBlower()){
                        return;
                    }

                    blowerClickRequested = true;
                    bellowAnimationAmount = 1.0f;
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

                void OnUiBackClicked(){
                    if (!isActive){
                        return;
                    }

                    ExitStation();
                }

                void OnUiInfoClicked(){
                    if (!isActive){
                        return;
                    }

                    ToggleCraftingTutorial();
                }

                void OnUiNextClicked(){
                    if (!isActive){
                        return;
                    }

                    if (currentStage == CraftingStage::Ingredients){
                        ConfirmIngredientStage();
                        return;
                    }

                    if (currentStage == CraftingStage::Heating){
                        FinishHeatingStage();
                        StartBottlingStage();
                        return;
                    }

                    if (currentStage == CraftingStage::Finished){
                        StartBottlingStage();
                        return;
                    }

                    if (currentStage == CraftingStage::Bottling && lastStagePotionPanelVisible){
                        ExitStation();
                        ShowDungeonPromptMessage();
                        return;
                    }
                }

                void OnInventoryNextPageClicked(){
                    if (!isActive || currentStage != CraftingStage::Ingredients){
                        return;
                    }

                    int maxPage = GetMaxInventoryPage();

                    inventoryPage++;

                    if (inventoryPage > maxPage){
                        inventoryPage = 0;
                    }

                    UpdateInventorySlotTexts();
                    UpdateVisibleInventoryItems();
                }

                void OnInventoryPreviousPageClicked(){
                    if (!isActive || currentStage != CraftingStage::Ingredients){
                        return;
                    }

                    int maxPage = GetMaxInventoryPage();

                    inventoryPage--;

                    if (inventoryPage < 0){
                        inventoryPage = maxPage;
                    }

                    UpdateInventorySlotTexts();
                    UpdateVisibleInventoryItems();
                }

          private:
                IngredientData CreateMainEffectIngredientData(
                    IngredientType ingredientType,
                    const std::string& displayName,
                    const std::string& effectId,
                    const glm::vec4& color
                ){
                    IngredientData data;
                    data.type = ingredientType;
                    data.displayName = displayName;
                    data.role = IngredientRole::MainEffect;
                    data.effectId = effectId;
                    data.modifierId = ModifierId::None;
                    data.value = 1.0f;
                    data.color = color;

                    return data;
                }

                IngredientData CreateModifierIngredientData(
                    IngredientType ingredientType,
                    const std::string& displayName,
                    const std::string& modifierId,
                    float value,
                    const glm::vec4& color
                ){
                    IngredientData data;
                    data.type = ingredientType;
                    data.displayName = displayName;
                    data.role = IngredientRole::Modifier;
                    data.effectId = EffectId::None;
                    data.modifierId = modifierId;
                    data.value = value;
                    data.color = color;

                    return data;
                }

                Font* LoadModelUiFont(){
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

                    return GetScene()->Resources()->Get<Font>(
                        "./res/fonts/OpenSans-Regular/OpenSans-Regular.json",
                        fontTexture
                    );
                }

                Text3D* CreateModelText(
                    SceneNode* parent,
                    const std::string& nodeName,
                    const std::string& text,
                    Font* font,
                    const glm::vec3& localPosition,
                    float scale
                ){
                    if (!parent || !font || !GetScene()){
                        return nullptr;
                    }

                    SceneNode* textNode =
                        GetScene()->CreateNode(parent, nodeName);

                    textNode->LocalTransform().Position() = localPosition;
                    textNode->GlobalTransform().Scale() = glm::vec3(scale);

                    auto* text3d =
                        textNode->AddObject<Text3D>(text, font);

                    text3d->color = glm::vec4(1.25f, 0.88f, 0.35f, 1.0f);
                    text3d->billboardMode = BillboardMode::Enabled;
                    text3d->SetAlignment(TextAlignment::Middle);

                    return text3d;
                }

                void CreateStageOneModelTexts(){
                    if (!GetScene()){
                        return;
                    }

                    Font* font = LoadModelUiFont();

                    const std::array<std::string, 4> cauldronSlots = {
                        "slot_bigger.004",
                        "slot_bigger.005",
                        "slot_bigger.006",
                        "slot_bigger.007"
                    };

                    for (int i = 0; i < 4; i++){
                        SceneNode* slotNode =
                            FindNodeRecursive(GetNode(), cauldronSlots[i]);

                        cauldronSlotTexts[i] = CreateModelText(
                            slotNode,
                            "CauldronSlotText_" + std::to_string(i),
                            "Empty",
                            font,
                            glm::vec3(0.0f, 0.04f, 0.06f),
                            0.035f
                        );
                    }

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_back"),
                        "StageOneBackLabel",
                        "Back",
                        font,
                        glm::vec3(0.0f, 0.04f, 0.05f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_info"),
                        "StageOneInfoLabel",
                        "Info",
                        font,
                        glm::vec3(0.0f, 0.04f, 0.05f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_next"),
                        "StageOneNextLabel",
                        "Brew",
                        font,
                        glm::vec3(0.0f, 0.04f, 0.05f),
                        0.055f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_next_page"),
                        "InventoryNextPageLabel",
                        "Down",
                        font,
                        glm::vec3(0.0f, 0.14f, 0.05f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_previous_page"),
                        "InventoryPreviousPageLabel",
                        "Up",
                        font,
                        glm::vec3(-0.03f, 0.14f, 0.05f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_back.001"),
                        "StageTwoBackLabel",
                        "Back",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_info.001"),
                        "StageTwoInfoLabel",
                        "Info",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_next.001"),
                        "StageTwoNextLabel",
                        "Next",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_back.002"),
                        "LastStageBackLabel",
                        "Back",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_info.002"),
                        "LastStageInfoLabel",
                        "Info",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.035f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_next.002"),
                        "LastStageNextLabel",
                        "Done",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.035f
                    );
                }

                void CreateStageTwoModelTexts(){
                    if (!GetScene()){
                        return;
                    }

                    Font* font = LoadModelUiFont();

                    stageTwoQualityText = CreateModelText(
                        qualityMeterNode,
                        "StageTwoQualityText",
                        "",
                        font,
                        glm::vec3(0.0f, 0.05f, 0.08f),
                        0.045f
                    );

                    stageTwoTemperatureText = CreateModelText(
                        bladeNode,
                        "StageTwoTemperatureText",
                        "",
                        font,
                        glm::vec3(0.0f, 0.06f, 0.12f),
                        0.04f
                    );

                    if (stageTwoTemperatureText){
                        stageTwoTemperatureText->SetEnabled(false);
                    }
                }

                void CreateLastStageModelTexts(){
                    if (!GetScene()){
                        return;
                    }

                    Font* font = LoadModelUiFont();

                    lastStagePotionInfoText = CreateModelText(
                        lastStagePotionPanelNode,
                        "LastStagePotionInfoText",
                        "",
                        font,
                        glm::vec3(0.0f, 0.05f, 0.08f),
                        0.024f
                    );
                }

                void UpdateLastStagePotionPanel(float deltaTime){
                    if (!lastStagePotionPanelNode || !lastStagePotionPanelVisible){
                        return;
                    }

                    lastStagePotionPanelSlide =
                        glm::clamp(
                            lastStagePotionPanelSlide + deltaTime * 1.75f,
                            0.0f,
                            1.0f
                        );

                    float smoothSlide =
                        lastStagePotionPanelSlide * lastStagePotionPanelSlide *
                        (3.0f - 2.0f * lastStagePotionPanelSlide);

                    lastStagePotionPanelNode->LocalTransform().Position() =
                        glm::mix(
                            lastStagePotionPanelHiddenPosition,
                            lastStagePotionPanelDefaultPosition,
                            smoothSlide
                        );

                    for (int i = 0; i < 5; i++){
                        if (!lastStagePotionStarNodes[i]){
                            continue;
                        }

                        lastStagePotionStarNodes[i]->LocalTransform().Position() =
                            glm::mix(
                                lastStagePotionStarHiddenPositions[i],
                                lastStagePotionStarDefaultPositions[i],
                                smoothSlide
                            );
                    }

                    if (lastStagePotionPanelSlide >= 1.0f && !lastStagePotionTextVisible){
                        if (lastStagePotionInfoText){
                            lastStagePotionInfoText->SetText(lastStagePotionResultText);
                        }

                        lastStagePotionTextVisible = true;
                    }
                }

                void ShowLastStagePotionPanel(){
                    lastStagePotionPanelVisible = true;
                    lastStagePotionTextVisible = false;
                    lastStagePotionResultText.clear();
                    lastStagePotionPanelSlide = 0.0f;

                    if (lastStagePotionPanelNode){
                        lastStagePotionPanelNode->SetEnabled(true);
                        lastStagePotionPanelNode->LocalTransform().Position() =
                            lastStagePotionPanelHiddenPosition;
                    }

                    for (int i = 0; i < 5; i++){
                        if (!lastStagePotionStarNodes[i]){
                            continue;
                        }

                        lastStagePotionStarNodes[i]->SetEnabled(false);
                        lastStagePotionStarNodes[i]->LocalTransform().Position() =
                            lastStagePotionStarHiddenPositions[i];
                    }

                    if (!lastStagePotionInfoText){
                        return;
                    }

                    lastStagePotionInfoText->SetText("");

                    std::ostringstream text;
                    text << std::fixed << std::setprecision(2);

                    if (!craftedPotionDataValid){
                        text << "potion failed\n";
                        text << "bottles=" << bottlingStage.GetFilledBottles() << "/" << bottlingStage.GetRequiredFilledBottles();

                        lastStagePotionResultText = text.str();
                        return;
                    }

                    float qualityPercent = 0.0f;

                    if (cauldron){
                        qualityPercent = cauldron->GetQualityPercent();
                    }

                    lastStagePotionVisibleStars = 0;

                    if (qualityPercent > 80.0f){
                        lastStagePotionVisibleStars = 5;
                    }
                    else if (qualityPercent > 60.0f){
                        lastStagePotionVisibleStars = 4;
                    }
                    else if (qualityPercent > 40.0f){
                        lastStagePotionVisibleStars = 3;
                    }
                    else if (qualityPercent > 20.0f){
                        lastStagePotionVisibleStars = 2;
                    }
                    else if (qualityPercent > 0.0f){
                        lastStagePotionVisibleStars = 1;
                    }

                    for (int i = 0; i < 5; i++){
                        if (!lastStagePotionStarNodes[i]){
                            continue;
                        }

                        lastStagePotionStarNodes[i]->SetEnabled(i < lastStagePotionVisibleStars);
                    }

                    text << "effect=" << craftedPotionData.primaryEffectId << "\n";

                    if (craftedPotionData.secondaryEffectId != EffectId::None){
                        text << "secondary=" << craftedPotionData.secondaryEffectId << "\n";
                    }

                    text << "radius=" << craftedPotionData.radius << " power=" << craftedPotionData.power << "\n";
                    text << "duration=" << craftedPotionData.duration << " bottles=" << bottlingStage.GetFilledBottles();

                    lastStagePotionResultText = text.str();
                }

                void UpdateQualityStars(){
                    float qualityPercent =
                        heatingStage.GetQualityPercent();

                    int visibleStars = 0;

                    if (qualityPercent > 80.0f){
                        visibleStars = 5;
                    }
                    else if (qualityPercent > 60.0f){
                        visibleStars = 4;
                    }
                    else if (qualityPercent > 40.0f){
                        visibleStars = 3;
                    }
                    else if (qualityPercent > 20.0f){
                        visibleStars = 2;
                    }
                    else if (qualityPercent > 0.0f){
                        visibleStars = 1;
                    }

                    for (int i = 0; i < 5; i++){
                        if (!qualityStarNodes[i]){
                            continue;
                        }

                        qualityStarNodes[i]->SetEnabled(i < visibleStars);
                    }
                }

                void UpdateBellowAnimation(float deltaTime){
                    if (!bellowNode){
                        return;
                    }

                    if (bellowAnimationAmount > 0.0f){
                        bellowAnimationAmount =
                            glm::clamp(
                                bellowAnimationAmount - deltaTime * 3.5f,
                                0.0f,
                                1.0f
                            );
                    }

                    glm::vec3 compressedScale = bellowDefaultScale;
                    compressedScale.y *= 0.55f;

                    bellowNode->LocalTransform().Scale() =
                        glm::mix(
                            bellowDefaultScale,
                            compressedScale,
                            bellowAnimationAmount
                        );
                }

                float GetThermometerDisplayMinTemperature() const{
                    if (thermometerUseHeatingRangeAsScale && thermometerArcStartsAtTargetMinimum){
                        return heatingStage.tempMin;
                    }

                    return thermometerMinTemperature;
                }

                float GetThermometerDisplayMaxTemperature() const{
                    if (!thermometerUseHeatingRangeAsScale){
                        return glm::max(
                            thermometerFallbackMaxTemperature,
                            GetThermometerDisplayMinTemperature() + 0.001f
                        );
                    }

                    float targetRange =
                        heatingStage.tempMax - heatingStage.tempMin;

                    if (targetRange < 0.001f){
                        targetRange = 0.001f;
                    }

                    float overheatRange =
                        targetRange * glm::max(
                            thermometerOverheatVisualRangeMultiplier,
                            0.001f
                        );

                    return glm::max(
                        heatingStage.tempMax + overheatRange,
                        GetThermometerDisplayMinTemperature() + 0.001f
                    );
                }

                float GetThermometerTemperature01(float temperature) const{
                    float displayMinTemperature =
                        GetThermometerDisplayMinTemperature();

                    float displayMaxTemperature =
                        GetThermometerDisplayMaxTemperature();

                    float range =
                        displayMaxTemperature - displayMinTemperature;

                    if (range < 0.001f){
                        range = 0.001f;
                    }

                    return glm::clamp(
                        (temperature - displayMinTemperature) / range,
                        0.0f,
                        1.0f
                    );
                }

                float GetThermometerAngleForTemperature(float temperature) const{
                    float temperature01 =
                        GetThermometerTemperature01(temperature);

                    return glm::mix(
                        thermometerNeedleMinAngleDegrees,
                        thermometerNeedleMaxAngleDegrees,
                        glm::clamp(temperature01, 0.0f, 1.0f)
                    );
                }

                glm::quat GetThermometerLocalRotationForTemperature(float temperature) const{
                    float angleDegrees =
                        GetThermometerAngleForTemperature(temperature);

                    return glm::angleAxis(
                        glm::radians(angleDegrees),
                        glm::vec3(0.0f, 0.0f, 1.0f)
                    );
                }

                bool IsThermometerNeedleInBlackRange() const{
                    return heatingStage.GetTemperature() > heatingStage.tempMax;
                }

                void CreateStageTwoTemperatureArcMaterial(){
                    if (!stageTwoColorNode || !GetScene()){
                        return;
                    }

                    ShaderProgram* arcShader =
                        ShaderProgram::Build()
                            .WithVertexShader("./res/shaders/gltf/crafting_temperature_arc.vert")
                            .WithPixelShader("./res/shaders/gltf/crafting_temperature_arc.frag")
                            .Link();

                    if (!arcShader){
                        return;
                    }

                    stageTwoTemperatureArcMaterial =
                        new Material(arcShader);

                    stageTwoTemperatureArcMaterial->name =
                        "StageTwoTemperatureArcMaterial";

                    std::vector<MeshRenderer*> colorRenderers;
                    CollectObjectsRecursive<MeshRenderer>(
                        stageTwoColorNode,
                        colorRenderers
                    );

                    for (MeshRenderer* renderer : colorRenderers){
                        if (!renderer){
                            continue;
                        }

                        for (int materialIndex = 0; materialIndex < renderer->GetMaterialCount(); materialIndex++){
                            renderer->SetMaterial(
                                stageTwoTemperatureArcMaterial,
                                materialIndex
                            );
                        }
                    }

                    UpdateStageTwoTemperatureArcMaterial();
                }

                void UpdateStageTwoTemperatureArcMaterial(){
                    if (!stageTwoTemperatureArcMaterial){
                        return;
                    }

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uGaugeLocalRadius",
                        thermometerArcLocalRadius
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uInnerRadius01",
                        thermometerArcInnerRadius01
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uOuterRadius01",
                        thermometerArcOuterRadius01
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uEdgeSoftness01",
                        thermometerArcEdgeSoftness01
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uAngleOffsetDegrees",
                        thermometerArcAngleOffsetDegrees
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uArcStartDegrees",
                        thermometerArcStartDegrees
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uArcEndDegrees",
                        thermometerArcEndDegrees
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uSafeStartFraction01",
                        GetThermometerTemperature01(heatingStage.tempMin)
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uSafeEndFraction01",
                        GetThermometerTemperature01(heatingStage.tempMax)
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uSafeColor",
                        thermometerSafeRangeColor
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uOverheatColor",
                        thermometerOverheatRangeColor
                    );

                    stageTwoTemperatureArcMaterial->SetValue(
                        "uBaseFillColor",
                        thermometerBaseFillColor
                    );
                }

                void UpdateStageTwoThermometer(){
                    if (bladeNode){
                        bladeNode->LocalTransform().Rotation() =
                            bladeDefaultLocalRotation *
                            GetThermometerLocalRotationForTemperature(
                                heatingStage.GetTemperature()
                            );
                    }

                    if (stageTwoColorNode){
                        stageTwoColorNode->LocalTransform().Rotation() =
                            stageTwoColorDefaultLocalRotation;
                    }

                    UpdateStageTwoTemperatureArcMaterial();
                }

                void UpdateHeatingModelUi(){
                    UpdateStageTwoThermometer();

                    std::ostringstream qualityText;
                    qualityText << std::fixed << std::setprecision(1);
                    qualityText << "Heating\n";
                    qualityText << "Quality: " << heatingStage.GetQualityPercent() << "%";

                    if (stageTwoQualityText){
                        stageTwoQualityText->SetText(qualityText.str());
                    }

                    if (stageTwoTemperatureText){
                        stageTwoTemperatureText->SetText("");
                    }

                    UpdateQualityStars();
                }

                void RefreshPlayerInventoryFromPersistent(){
                    PotionInventory::EnsureStartingIngredients();
                    playerInventory = PotionInventory::GetOwnedIngredientDefinitions();
                    ClampInventoryPage();
                }

                int GetMaxInventoryPage() const{
                    if (playerInventory.empty()){
                        return 0;
                    }

                    return static_cast<int>((playerInventory.size() - 1) / 4);
                }

                void ClampInventoryPage(){
                    int maxPage = GetMaxInventoryPage();

                    if (inventoryPage < 0){
                        inventoryPage = 0;
                    }

                    if (inventoryPage > maxPage){
                        inventoryPage = maxPage;
                    }
                }

                int GetInventoryIndexForSlot(int slotIndex) const{
                    return inventoryPage * 4 + slotIndex;
                }

                SceneNode* GetInventorySlotNode(int slotIndex){
                    const std::array<std::string, 4> inventorySlots = {
                        "slot_bigger",
                        "slot_bigger.001",
                        "slot_bigger.002",
                        "slot_bigger.003"
                    };

                    if (slotIndex < 0 || slotIndex >= static_cast<int>(inventorySlots.size())){
                        return nullptr;
                    }

                    return FindNodeRecursive(GetNode(), inventorySlots[slotIndex]);
                }

                DraggableCraftingItem* FindInventoryItemByKey(
                    const std::string& inventoryKey,
                    const std::vector<DraggableCraftingItem*>& items
                ){
                    for (auto* item : items){
                        if (!item){
                            continue;
                        }

                        if (item->inventoryKey == inventoryKey){
                            return item;
                        }
                    }

                    return nullptr;
                }

                void UpdateInventorySlotTexts(){
                    ClampInventoryPage();

                    for (int i = 0; i < 4; i++){
                        if (!inventorySlotTexts[i]){
                            continue;
                        }

                        int inventoryIndex = GetInventoryIndexForSlot(i);

                        if (inventoryIndex < 0 || inventoryIndex >= static_cast<int>(playerInventory.size())){
                            inventorySlotTexts[i]->SetText("Empty");
                            continue;
                        }

                        const PotionInventory::IngredientInventoryEntry& entry =
                            playerInventory[inventoryIndex];

                        std::ostringstream text;
                        text << entry.displayName;
                        text << " x" << PotionInventory::GetIngredientCount(entry.inventoryKey);

                        inventorySlotTexts[i]->SetText(text.str());
                    }
                }

                void UpdateVisibleInventoryItems(){
                    SceneNode* ingredientsRootNode = GetIngredientsRootNode();

                    if (!ingredientsRootNode){
                        return;
                    }

                    std::vector<DraggableCraftingItem*> items;
                    CollectObjectsRecursive<DraggableCraftingItem>(ingredientsRootNode, items);

                    for (auto* item : items){
                        if (!item){
                            continue;
                        }

                        item->SetInventoryVisible(false);
                    }

                    ClampInventoryPage();

                    for (int slotIndex = 0; slotIndex < 4; slotIndex++){
                        int inventoryIndex = GetInventoryIndexForSlot(slotIndex);

                        if (inventoryIndex < 0 || inventoryIndex >= static_cast<int>(playerInventory.size())){
                            continue;
                        }

                        const PotionInventory::IngredientInventoryEntry& entry =
                            playerInventory[inventoryIndex];

                        DraggableCraftingItem* item =
                            FindInventoryItemByKey(entry.inventoryKey, items);

                        if (!item){
                            spdlog::warn(
                                "CraftingStation: inventory item '{}' has no scene model.",
                                entry.displayName
                            );
                            continue;
                        }

                        SceneNode* slotNode = GetInventorySlotNode(slotIndex);

                        if (!slotNode){
                            continue;
                        }

                        item->data = entry.data;
                        item->data.inventoryKey = entry.inventoryKey;
                        item->inventoryKey = entry.inventoryKey;
                        item->SetStartPosition(
                            slotNode->GlobalTransform().Position().Value()
                        );
                        item->SetInventoryVisible(true);
                    }
                }

                void UpdateCauldronSlotTexts(){
                    const std::vector<IngredientData>* ingredients = nullptr;

                    if (cauldron){
                        ingredients = &cauldron->GetIngredients();
                    }

                    for (int i = 0; i < 4; i++){
                        if (!cauldronSlotTexts[i]){
                            continue;
                        }

                        if (!ingredients || i >= static_cast<int>(ingredients->size())){
                            cauldronSlotTexts[i]->SetText("Empty");
                            continue;
                        }

                        cauldronSlotTexts[i]->SetText((*ingredients)[i].displayName);
                    }
                }

                void UpdateIngredientsStage(){
                    bool canConfirm =
                        cauldron && cauldron->CanConfirm();

                    SetLidInteractionEnabled(canConfirm);
                    UpdateCauldronSlotTexts();
                }

                void UpdateHeatingStage(){
                    float deltaTime = Time::Delta();

                    bool blowerClicked = blowerClickRequested;
                    blowerClickRequested = false;

                    bool finished =
                        heatingStage.Update(deltaTime, blowerClicked);

                    UpdateBellowAnimation(deltaTime);
                    UpdateHeatingModelUi();

                    if (finished){
                        FinishHeatingStage();
                    }
                }

                void UpdateBottlingStage(){
                    float deltaTime = Time::Delta();

                    bool finished = bottlingStage.Update(deltaTime);

                    UpdateLastStagePotionPanel(deltaTime);

                    if (finished && !lastStagePotionPanelVisible){
                        FinishBottlingStage();
                    }
                }

                void StartHeatingStage(){
                    currentStage = CraftingStage::Heating;

                    if (stageOneUiNode){
                        stageOneUiNode->SetEnabled(false);
                    }

                    if (stageTwoUiNode){
                        stageTwoUiNode->SetEnabled(true);
                    }

                    if (lastStageUiNode){
                        lastStageUiNode->SetEnabled(false);
                    }

                    if (stageOneCameraLightNode){
                        stageOneCameraLightNode->SetEnabled(false);
                    }

                    if (stageTwoCameraLightNode){
                        stageTwoCameraLightNode->SetEnabled(true);
                    }

                    if (lastStageCameraLightNode){
                        lastStageCameraLightNode->SetEnabled(false);
                    }

                    heatingStage.Start();

                    bellowAnimationAmount = 0.0f;

                    if (bellowNode){
                        bellowNode->LocalTransform().Scale() =
                            bellowDefaultScale;
                    }

                    SetHeatingUiEnabled(false);
                    UpdateHeatingModelUi();

                    SetIngredientsEnabled(false);
                    SetDragInteractorEnabled(true);
                    SetDoorInteractionEnabled(false);
                    SetBlowerInteractionEnabled(true);
                }

                void FinishHeatingStage(){
                    currentStage = CraftingStage::Finished;

                    if (stageOneUiNode){
                        stageOneUiNode->SetEnabled(false);
                    }

                    if (stageTwoUiNode){
                        stageTwoUiNode->SetEnabled(true);
                    }

                    if (lastStageUiNode){
                        lastStageUiNode->SetEnabled(false);
                    }

                    if (stageOneCameraLightNode){
                        stageOneCameraLightNode->SetEnabled(false);
                    }

                    if (stageTwoCameraLightNode){
                        stageTwoCameraLightNode->SetEnabled(true);
                    }

                    if (lastStageCameraLightNode){
                        lastStageCameraLightNode->SetEnabled(false);
                    }

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

                    if (stageOneUiNode){
                        stageOneUiNode->SetEnabled(false);
                    }

                    if (stageTwoUiNode){
                        stageTwoUiNode->SetEnabled(false);
                    }

                    if (lastStageUiNode){
                        lastStageUiNode->SetEnabled(true);
                    }

                    if (stageOneCameraLightNode){
                        stageOneCameraLightNode->SetEnabled(false);
                    }

                    if (stageTwoCameraLightNode){
                        stageTwoCameraLightNode->SetEnabled(false);
                    }

                    if (lastStageCameraLightNode){
                        lastStageCameraLightNode->SetEnabled(true);
                    }

                    craftedPotionDataValid = false;
                    lastStagePotionPanelVisible = false;
                    lastStagePotionTextVisible = false;
                    lastStagePotionResultText.clear();
                    lastStagePotionPanelSlide = 0.0f;

                    if (lastStagePotionPanelNode){
                        lastStagePotionPanelNode->SetEnabled(false);
                        lastStagePotionPanelNode->LocalTransform().Position() =
                            lastStagePotionPanelHiddenPosition;
                    }

                    if (lastStagePotionInfoText){
                        lastStagePotionInfoText->SetText("");
                    }

                    lastStagePotionVisibleStars = 0;

                    for (int i = 0; i < 5; i++){
                        if (!lastStagePotionStarNodes[i]){
                            continue;
                        }

                        lastStagePotionStarNodes[i]->SetEnabled(false);
                        lastStagePotionStarNodes[i]->LocalTransform().Position() =
                            lastStagePotionStarHiddenPositions[i];
                    }

                    SetHeatingUiEnabled(false);

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
                    bottlingStage.Stop();

                    SaveCraftedPotion();
                    ShowLastStagePotionPanel();
                }

                void SaveCraftedPotion(){
                    craftedPotionDataValid = false;

                    if (!cauldron || !bottlingStage.HasEnoughFilledBottles()){
                        return;
                    }

                    craftedPotionData =
                        CraftingRecipeChecker::BuildCraftedPotion(
                            cauldron->GetIngredients(),
                            cauldron->GetQualityPercent()
                        );

                    craftedPotionDataValid = true;

                    PotionInventory::SaveLastCraftedPotion(
                        craftedPotionData,
                        bottlingStage.GetFilledBottles()
                    );

                    spdlog::error(
                        "CraftingStation: crafted '{}' effect={} secondary={} radius={:.2f} duration={:.2f} power={:.2f} bottles={}",
                        craftedPotionData.recipeName,
                        craftedPotionData.primaryEffectId,
                        craftedPotionData.secondaryEffectId,
                        craftedPotionData.radius,
                        craftedPotionData.duration,
                        craftedPotionData.power,
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

                    lidNode->LocalTransform().Position() =
                        lidClosedLocalPosition;

                    lidNode->LocalTransform().Rotation() =
                        glm::quat(glm::radians(glm::vec3(0.0f, -120.0f, 0.0f)));

                    SyncPhysicsBodyToNode(lidNode);

                    if (lidHitboxNode){
                        if (!lidHitboxIsChildOfLid){
                            lidHitboxNode->LocalTransform().Position() =
                                lidHitboxClosedLocalPosition;
                        }

                        SyncPhysicsBodyToNode(lidHitboxNode);
                    }
                }

                void CloseLid(){
                    if (!lidNode){
                        return;
                    }

                    lidNode->LocalTransform().Position() =
                        lidClosedLocalPosition;

                    lidNode->LocalTransform().Rotation() =
                        glm::quat(glm::radians(glm::vec3(0.0f, -90.0f, 0.0f)));

                    SyncPhysicsBodyToNode(lidNode);

                    if (lidHitboxNode){
                        if (!lidHitboxIsChildOfLid){
                            lidHitboxNode->LocalTransform().Position() =
                                lidHitboxClosedLocalPosition;
                        }

                        SyncPhysicsBodyToNode(lidHitboxNode);
                    }
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
                    if (FocusCameraOnBlenderCamera(ingredientCameraNodeName)){
                        return;
                    }

                    FocusCameraOnStage(
                        ingredientCameraPointNodeName,
                        ingredientCameraTargetNodeName
                    );
                }

                void FocusCameraOnHeatingStage(){
                    if (FocusCameraOnBlenderCamera(heatingCameraNodeName)){
                        return;
                    }

                    FocusCameraOnStage(
                        heatingCameraPointNodeName,
                        heatingCameraTargetNodeName
                    );
                }

                void FocusCameraOnBottlingStage(){
                    if (FocusCameraOnBlenderCamera(bottlingCameraNodeName)){
                        return;
                    }

                    FocusCameraOnStage(
                        bottlingCameraPointNodeName,
                        bottlingCameraTargetNodeName
                    );
                }

                SceneNode* FindCameraNodeWithCameraObject(SceneNode* rootNode){
                    if (!rootNode){
                        return nullptr;
                    }

                    if (rootNode->GetObject<Camera>()){
                        return rootNode;
                    }

                    for (SceneNode* child : rootNode->GetChildren()){
                        if (SceneNode* foundNode = FindCameraNodeWithCameraObject(child)){
                            return foundNode;
                        }
                    }

                    return nullptr;
                }

                bool FocusCameraOnBlenderCamera(const std::string& blenderCameraNodeName){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        spdlog::warn("CraftingStation: runtime camera node not found.");
                        return false;
                    }

                    SceneNode* blenderCameraRootNode =
                        FindNodeRecursive(GetNode(), blenderCameraNodeName);

                    if (!blenderCameraRootNode){
                        spdlog::warn(
                            "CraftingStation: camera root '{}' not found under station '{}'.",
                            blenderCameraNodeName,
                            GetNode() ? GetNode()->GetName() : "null"
                        );

                        return false;
                    }

                    SceneNode* blenderCameraDataNode =
                        FindCameraNodeWithCameraObject(blenderCameraRootNode);

                    Camera* runtimeCamera =
                        cameraNode->GetObject<Camera>();

                    Camera* blenderCamera = nullptr;

                    if (blenderCameraDataNode){
                        blenderCamera = blenderCameraDataNode->GetObject<Camera>();
                    }

                    glm::vec3 cameraPosition =
                        blenderCameraRootNode->GlobalTransform().Position().Value();

                    glm::quat cameraRotation =
                        blenderCameraRootNode->GlobalTransform().Rotation().Value() *
                        glm::quat(glm::radians(glm::vec3(180.0f, 0.0f, 0.0f)));

                    cameraNode->GlobalTransform().Position() =
                        cameraPosition;

                    cameraNode->GlobalTransform().Rotation() =
                        cameraRotation;

                    glm::vec3 cameraForward =
                        cameraNode->GlobalTransform().Forward();

                    if (runtimeCamera && blenderCamera){
                        runtimeCamera->MakePerspective(
                            blenderCamera->GetFov(),
                            blenderCamera->GetAspectRatio(),
                            blenderCamera->GetNearPlane(),
                            blenderCamera->GetFarPlane()
                        );
                    }
                    else if (runtimeCamera){
                        runtimeCamera->MakePerspective(
                            22.895f,
                            16.0f / 9.0f,
                            0.1f,
                            1000.0f
                        );
                    }

                    return true;
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

                SceneNode* GetGameplayHudNode(){
                    if (gameplayHudNode){
                        return gameplayHudNode;
                    }

                    if (!GetScene()){
                        return nullptr;
                    }

                    gameplayHudNode =
                        GetScene()->FindNode("HUD");

                    return gameplayHudNode;
                }

                void HideGameplayHudForCrafting(){
                    if (gameplayHudVisibilitySaved){
                        return;
                    }

                    SceneNode* hudNode =
                        GetGameplayHudNode();

                    if (!hudNode){
                        return;
                    }

                    gameplayHudWasEnabled =
                        hudNode->EnabledSelf();

                    gameplayHudVisibilitySaved = true;

                    hudNode->SetEnabled(false);
                }

                void RestoreGameplayHudAfterCrafting(){
                    if (!gameplayHudVisibilitySaved){
                        return;
                    }

                    SceneNode* hudNode =
                        GetGameplayHudNode();

                    if (hudNode){
                        hudNode->SetEnabled(gameplayHudWasEnabled);
                    }

                    gameplayHudVisibilitySaved = false;
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


                void CreateCraftingTutorialUi(){
                    if (tutorialTextPanelNode || !GetScene()){
                        return;
                    }

                    Font* font = LoadModelUiFont();

                    if (!font){
                        return;
                    }

                    for (int i = 0; i < 4; i++){
                        SceneNode* panelNode =
                            GetScene()->CreateNode(
                                "Crafting Tutorial Dim Panel " + std::to_string(i)
                            );

                        tutorialDimPanelNodes[i] = panelNode;
                        tutorialDimPanelLayouts[i] = panelNode->AddObject<UiLayout>(
                            glm::ivec2(1, 1),
                            glm::ivec2(0, 0),
                            500,
                            AnchorPoint::TopLeft
                        );

                        panelNode->AddObject<UiVisual>(
                            glm::vec4(0.0f, 0.0f, 0.0f, 0.68f)
                        );

                        SceneNode* borderNode =
                            GetScene()->CreateNode(
                                "Crafting Tutorial Border Panel " + std::to_string(i)
                            );

                        tutorialBorderPanelNodes[i] = borderNode;
                        tutorialBorderPanelLayouts[i] = borderNode->AddObject<UiLayout>(
                            glm::ivec2(1, 1),
                            glm::ivec2(0, 0),
                            501,
                            AnchorPoint::TopLeft
                        );

                        borderNode->AddObject<UiVisual>(
                            glm::vec4(1.0f, 0.92f, 0.55f, 0.55f)
                        );
                    }

                    tutorialTextPanelNode =
                        GetScene()->CreateNode("Crafting Tutorial Text Panel");

                    tutorialTextPanelLayout =
                        tutorialTextPanelNode->AddObject<UiLayout>(
                            glm::ivec2(820, 125),
                            glm::ivec2(0, 0),
                            510,
                            AnchorPoint::TopLeft
                        );

                    tutorialTextPanelNode->AddObject<UiVisual>(
                        glm::vec4(0.04f, 0.03f, 0.02f, 0.88f)
                    );

                    SceneNode* textNode =
                        GetScene()->CreateNode(
                            tutorialTextPanelNode,
                            "Crafting Tutorial Text"
                        );

                    textNode->AddObject<UiLayout>(
                        glm::ivec2(780, 95),
                        glm::ivec2(20, 15),
                        511,
                        AnchorPoint::TopLeft
                    );

                    tutorialText =
                        textNode->AddObject<UiText>("", font);

                    tutorialText->fontSize = 28.0f;
                    tutorialText->color = glm::vec4(1.0f, 0.93f, 0.72f, 1.0f);
                    tutorialText->alignment = TextAlignment::Middle;
                    tutorialText->verticalAlignment = TextVerticalAlignment::Middle;
                    tutorialText->maxWidth = 760.0f;
                }

                void ToggleCraftingTutorial(){
                    if (craftingTutorialVisible){
                        HideCraftingTutorial();
                        return;
                    }

                    craftingTutorialVisible = true;
                    activeTutorialFocus = TutorialFocus::None;
                    SetCraftingTutorialNodesEnabled(true);
                    UpdateCraftingTutorial();
                }

                void HideCraftingTutorial(){
                    craftingTutorialVisible = false;
                    activeTutorialFocus = TutorialFocus::None;
                    SetCraftingTutorialNodesEnabled(false);

                    if (tutorialText){
                        tutorialText->text = "";
                    }
                }

                void SetCraftingTutorialNodesEnabled(bool enabled){
                    for (SceneNode* node : tutorialDimPanelNodes){
                        if (node){
                            node->SetEnabled(enabled);
                        }
                    }

                    for (SceneNode* node : tutorialBorderPanelNodes){
                        if (node){
                            node->SetEnabled(enabled);
                        }
                    }

                    if (tutorialTextPanelNode){
                        tutorialTextPanelNode->SetEnabled(enabled);
                    }
                }

                bool ProjectWorldToTutorialScreen(
                    const glm::vec3& worldPosition,
                    glm::vec2& outVirtualScreenPosition,
                    glm::vec2& outVirtualScreenSize
                ) const{
                    if (!GetScene() || !GetScene()->GetGraphics()){
                        return false;
                    }

                    SceneNode* cameraNode = GetScene()->FindNode("Camera Node");

                    if (!cameraNode){
                        return false;
                    }

                    Camera* camera = cameraNode->GetObject<Camera>();

                    if (!camera){
                        return false;
                    }

                    glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();

                    if (resolution.x <= 1.0f || resolution.y <= 1.0f){
                        return false;
                    }

                    glm::vec4 clipPosition =
                        camera->ProjectionMatrix() *
                        camera->ViewMatrix() *
                        glm::vec4(worldPosition, 1.0f);

                    if (clipPosition.w <= 0.0001f){
                        return false;
                    }

                    glm::vec3 ndc = glm::vec3(clipPosition) / clipPosition.w;

                    if (ndc.x < -1.2f || ndc.x > 1.2f ||
                        ndc.y < -1.2f || ndc.y > 1.2f ||
                        ndc.z < -1.2f || ndc.z > 1.2f){
                        return false;
                    }

                    glm::vec2 pixelPosition;
                    pixelPosition.x = (ndc.x * 0.5f + 0.5f) * resolution.x;
                    pixelPosition.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * resolution.y;

                    float scaleFactor = resolution.y / 1080.0f;

                    if (scaleFactor <= 0.0001f){
                        return false;
                    }

                    outVirtualScreenPosition = pixelPosition / scaleFactor;
                    outVirtualScreenSize = resolution / scaleFactor;
                    return true;
                }

                bool ProjectNodeToTutorialScreen(
                    SceneNode* node,
                    glm::vec2& outVirtualScreenPosition,
                    glm::vec2& outVirtualScreenSize
                ) const{
                    if (!node){
                        return false;
                    }

                    return ProjectWorldToTutorialScreen(
                        node->GlobalTransform().Position().Value(),
                        outVirtualScreenPosition,
                        outVirtualScreenSize
                    );
                }

                bool ProjectNodesCenterToTutorialScreen(
                    const std::vector<SceneNode*>& nodes,
                    glm::vec2& outVirtualScreenPosition,
                    glm::vec2& outVirtualScreenSize
                ) const{
                    glm::vec2 sum = glm::vec2(0.0f);
                    glm::vec2 screenSize = glm::vec2(0.0f);
                    int projectedNodes = 0;

                    for (SceneNode* node : nodes){
                        glm::vec2 projectedPosition;
                        glm::vec2 projectedScreenSize;

                        if (!ProjectNodeToTutorialScreen(node, projectedPosition, projectedScreenSize)){
                            continue;
                        }

                        sum += projectedPosition;
                        screenSize = projectedScreenSize;
                        projectedNodes++;
                    }

                    if (projectedNodes <= 0){
                        return false;
                    }

                    outVirtualScreenPosition = sum / static_cast<float>(projectedNodes);
                    outVirtualScreenSize = screenSize;
                    return true;
                }

                bool ProjectNodesBoundsToTutorialScreen(
                    const std::vector<SceneNode*>& nodes,
                    glm::vec4& outBounds,
                    glm::vec2& outVirtualScreenSize
                ) const{
                    bool hasProjectedNode = false;
                    glm::vec2 minPosition = glm::vec2(0.0f);
                    glm::vec2 maxPosition = glm::vec2(0.0f);
                    glm::vec2 screenSize = glm::vec2(0.0f);

                    for (SceneNode* node : nodes){
                        glm::vec2 projectedPosition;
                        glm::vec2 projectedScreenSize;

                        if (!ProjectNodeToTutorialScreen(node, projectedPosition, projectedScreenSize)){
                            continue;
                        }

                        if (!hasProjectedNode){
                            minPosition = projectedPosition;
                            maxPosition = projectedPosition;
                            hasProjectedNode = true;
                        }
                        else{
                            minPosition.x = glm::min(minPosition.x, projectedPosition.x);
                            minPosition.y = glm::min(minPosition.y, projectedPosition.y);
                            maxPosition.x = glm::max(maxPosition.x, projectedPosition.x);
                            maxPosition.y = glm::max(maxPosition.y, projectedPosition.y);
                        }

                        screenSize = projectedScreenSize;
                    }

                    if (!hasProjectedNode){
                        return false;
                    }

                    outBounds = glm::vec4(
                        minPosition.x,
                        minPosition.y,
                        maxPosition.x,
                        maxPosition.y
                    );
                    outVirtualScreenSize = screenSize;
                    return true;
                }

                TutorialFocus GetCurrentTutorialFocus() const{
                    if (currentStage == CraftingStage::Ingredients){
                        if (IsAnyIngredientBeingDragged()){
                            return TutorialFocus::IngredientCauldron;
                        }

                        if (cauldron && cauldron->CanConfirm()){
                            return TutorialFocus::IngredientConfirm;
                        }

                        return TutorialFocus::IngredientInventory;
                    }

                    if (currentStage == CraftingStage::Heating){
                        return TutorialFocus::HeatingBlower;
                    }

                    if (currentStage == CraftingStage::Finished){
                        return TutorialFocus::HeatingDoor;
                    }

                    if (currentStage == CraftingStage::Bottling){
                        if (lastStagePotionPanelVisible){
                            return TutorialFocus::BottlingDone;
                        }

                        return TutorialFocus::BottlingValve;
                    }

                    return TutorialFocus::None;
                }

                std::string GetTutorialText(TutorialFocus focus) const{
                    switch (focus){
                        case TutorialFocus::IngredientInventory:
                            return "These are your ingredients. Grab one with the mouse and move it into the cauldron.";

                        case TutorialFocus::IngredientCauldron:
                            return "Drop the ingredient into the cauldron opening to add it to the potion.";

                        case TutorialFocus::IngredientConfirm:
                            return "Press Brew when all ingredients are inside.";

                        case TutorialFocus::HeatingBlower:
                            return "Use the bellows to heat the potion and keep the temperature in the green range.";

                        case TutorialFocus::HeatingDoor:
                            return "Heating is complete. Press Next to continue to bottling.";

                        case TutorialFocus::BottlingValve:
                            return "Use the valve to fill bottles with the finished potion.";

                        case TutorialFocus::BottlingDone:
                            return "Review your potion result, then press Done to finish crafting.";

                        case TutorialFocus::None:
                        default:
                            return "";
                    }
                }

                bool IsAnyIngredientBeingDragged() const{
                    SceneNode* ingredientsRootNode = FindNodeRecursive(GetNode(), "Crafting Ingredients");

                    if (!ingredientsRootNode){
                        return false;
                    }

                    std::vector<DraggableCraftingItem*> items;
                    CollectObjectsRecursive<DraggableCraftingItem>(ingredientsRootNode, items);

                    for (const DraggableCraftingItem* item : items){
                        if (item && item->isDragged){
                            return true;
                        }
                    }

                    return false;
                }

                bool GetTutorialFocusHole(
                    TutorialFocus focus,
                    glm::vec4& outHole,
                    glm::vec2& outScreenSize
                ){
                    glm::vec2 center = glm::vec2(0.0f);
                    glm::vec2 screenSize = glm::vec2(1920.0f, 1080.0f);
                    glm::vec2 holeSize = glm::vec2(520.0f, 360.0f);
                    bool projected = false;

                    if (focus == TutorialFocus::IngredientInventory){
                        std::vector<SceneNode*> nodes;

                        for (int i = 0; i < 4; i++){
                            if (SceneNode* slotNode = GetInventorySlotNode(i)){
                                nodes.push_back(slotNode);
                            }
                        }

                        projected = ProjectNodesCenterToTutorialScreen(nodes, center, screenSize);
                        holeSize = glm::vec2(680.0f, 420.0f);
                    }
                    else if (focus == TutorialFocus::IngredientCauldron){
                        SceneNode* focusNode = FindNodeRecursive(GetNode(), "Cauldron");

                        if (!focusNode){
                            focusNode = FindNodeRecursive(GetNode(), "slot_bigger.004");
                        }

                        projected = ProjectNodeToTutorialScreen(focusNode, center, screenSize);
                        holeSize = glm::vec2(560.0f, 440.0f);
                    }
                    else if (focus == TutorialFocus::IngredientConfirm){
                        SceneNode* brewButton = FindNodeRecursive(GetNode(), "przycisk_next");
                        projected = ProjectNodeToTutorialScreen(brewButton, center, screenSize);
                        holeSize = glm::vec2(260.0f, 150.0f);
                    }
                    else if (focus == TutorialFocus::HeatingBlower){
                        SceneNode* focusNode = bellowNode ? bellowNode : blowerHitboxNode;
                        projected = ProjectNodeToTutorialScreen(focusNode, center, screenSize);
                        holeSize = glm::vec2(620.0f, 430.0f);
                    }
                    else if (focus == TutorialFocus::HeatingDoor){
                        std::vector<SceneNode*> nodes;

                        if (doorHitboxNode){
                            nodes.push_back(doorHitboxNode);
                        }

                        if (SceneNode* nextButton = FindNodeRecursive(GetNode(), "przycisk_next.001")){
                            nodes.push_back(nextButton);
                        }

                        projected = ProjectNodesCenterToTutorialScreen(nodes, center, screenSize);
                        holeSize = glm::vec2(680.0f, 360.0f);
                    }
                    else if (focus == TutorialFocus::BottlingValve){
                        SceneNode* focusNode = valveHitboxNode;

                        if (!focusNode){
                            focusNode = FindNodeRecursive(GetNode(), "Knob_One");
                        }

                        projected = ProjectNodeToTutorialScreen(focusNode, center, screenSize);

                        if (projected){
                            const float holeWidth = 1080.0f;
                            const float topMargin = 140.0f;
                            float left = center.x - holeWidth * 0.5f;
                            float top = glm::max(0.0f, center.y - topMargin);

                            left = glm::clamp(left, 0.0f, glm::max(0.0f, screenSize.x - holeWidth));

                            outHole.x = left;
                            outHole.y = top;
                            outHole.z = glm::min(holeWidth, screenSize.x);
                            outHole.w = glm::max(0.0f, screenSize.y - top);
                            outScreenSize = screenSize;
                            return true;
                        }
                    }
                    else if (focus == TutorialFocus::BottlingDone){
                        std::vector<SceneNode*> nodes;

                        if (lastStagePotionPanelNode){
                            nodes.push_back(lastStagePotionPanelNode);
                        }

                        if (SceneNode* doneButton = FindNodeRecursive(GetNode(), "przycisk_next.002")){
                            nodes.push_back(doneButton);
                        }

                        glm::vec4 bounds = glm::vec4(0.0f);
                        projected = ProjectNodesBoundsToTutorialScreen(nodes, bounds, screenSize);

                        if (projected){
                            float left = bounds.x - 260.0f;
                            float top = bounds.y - 190.0f;
                            float right = bounds.z + 280.0f;

                            left = glm::clamp(left, 0.0f, screenSize.x);
                            top = glm::clamp(top, 0.0f, screenSize.y);
                            right = glm::clamp(right, left, screenSize.x);

                            outHole.x = left;
                            outHole.y = top;
                            outHole.z = glm::max(0.0f, right - left);
                            outHole.w = glm::max(0.0f, screenSize.y - top);
                            outScreenSize = screenSize;
                            return true;
                        }
                    }

                    if (!projected){
                        if (!GetScene() || !GetScene()->GetGraphics()){
                            return false;
                        }

                        glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
                        float scaleFactor = resolution.y / 1080.0f;

                        if (scaleFactor <= 0.0001f){
                            return false;
                        }

                        screenSize = resolution / scaleFactor;
                        center = screenSize * 0.5f;
                    }

                    float padding = 18.0f;
                    holeSize += glm::vec2(padding * 2.0f);

                    outHole.x = glm::clamp(center.x - holeSize.x * 0.5f, 0.0f, glm::max(0.0f, screenSize.x - holeSize.x));
                    outHole.y = glm::clamp(center.y - holeSize.y * 0.5f, 0.0f, glm::max(0.0f, screenSize.y - holeSize.y));
                    outHole.z = glm::min(holeSize.x, screenSize.x);
                    outHole.w = glm::min(holeSize.y, screenSize.y);
                    outScreenSize = screenSize;
                    return true;
                }

                void SetTutorialLayout(
                    UiLayout* layout,
                    const glm::vec2& position,
                    const glm::vec2& size
                ){
                    if (!layout){
                        return;
                    }

                    layout->offset = glm::ivec2(
                        static_cast<int>(position.x),
                        static_cast<int>(position.y)
                    );

                    layout->size = glm::ivec2(
                        glm::max(0, static_cast<int>(size.x)),
                        glm::max(0, static_cast<int>(size.y))
                    );
                }

                void UpdateTutorialDimPanels(
                    const glm::vec4& hole,
                    const glm::vec2& screenSize
                ){
                    float left = hole.x;
                    float top = hole.y;
                    float right = hole.x + hole.z;
                    float bottom = hole.y + hole.w;

                    SetTutorialLayout(
                        tutorialDimPanelLayouts[0],
                        glm::vec2(0.0f, 0.0f),
                        glm::vec2(screenSize.x, top)
                    );

                    SetTutorialLayout(
                        tutorialDimPanelLayouts[1],
                        glm::vec2(0.0f, bottom),
                        glm::vec2(screenSize.x, glm::max(0.0f, screenSize.y - bottom))
                    );

                    SetTutorialLayout(
                        tutorialDimPanelLayouts[2],
                        glm::vec2(0.0f, top),
                        glm::vec2(left, hole.w)
                    );

                    SetTutorialLayout(
                        tutorialDimPanelLayouts[3],
                        glm::vec2(right, top),
                        glm::vec2(glm::max(0.0f, screenSize.x - right), hole.w)
                    );
                }

                void UpdateTutorialBorderPanels(const glm::vec4& hole){
                    const float borderThickness = 6.0f;
                    float left = hole.x;
                    float top = hole.y;
                    float right = hole.x + hole.z;
                    float bottom = hole.y + hole.w;

                    SetTutorialLayout(
                        tutorialBorderPanelLayouts[0],
                        glm::vec2(left, glm::max(0.0f, top - borderThickness)),
                        glm::vec2(hole.z, borderThickness)
                    );

                    SetTutorialLayout(
                        tutorialBorderPanelLayouts[1],
                        glm::vec2(left, bottom),
                        glm::vec2(hole.z, borderThickness)
                    );

                    SetTutorialLayout(
                        tutorialBorderPanelLayouts[2],
                        glm::vec2(glm::max(0.0f, left - borderThickness), top),
                        glm::vec2(borderThickness, hole.w)
                    );

                    SetTutorialLayout(
                        tutorialBorderPanelLayouts[3],
                        glm::vec2(right, top),
                        glm::vec2(borderThickness, hole.w)
                    );
                }

                void UpdateTutorialTextPanel(
                    const glm::vec4& hole,
                    const glm::vec2& screenSize,
                    const std::string& text
                ){
                    if (!tutorialTextPanelLayout || !tutorialText){
                        return;
                    }

                    tutorialText->text = text;

                    glm::vec2 panelSize = glm::vec2(820.0f, 125.0f);
                    float margin = 24.0f;
                    float panelX = glm::clamp(
                        screenSize.x * 0.5f - panelSize.x * 0.5f,
                        margin,
                        glm::max(margin, screenSize.x - panelSize.x - margin)
                    );

                    float panelY = margin;

                    SetTutorialLayout(
                        tutorialTextPanelLayout,
                        glm::vec2(panelX, panelY),
                        panelSize
                    );
                }

                void UpdateCraftingTutorial(){
                    if (!craftingTutorialVisible){
                        return;
                    }

                    if (!tutorialTextPanelNode){
                        CreateCraftingTutorialUi();
                    }

                    if (!tutorialTextPanelNode){
                        return;
                    }

                    TutorialFocus focus = GetCurrentTutorialFocus();

                    if (focus == TutorialFocus::None){
                        HideCraftingTutorial();
                        return;
                    }

                    activeTutorialFocus = focus;

                    glm::vec4 hole;
                    glm::vec2 screenSize;

                    if (!GetTutorialFocusHole(focus, hole, screenSize)){
                        HideCraftingTutorial();
                        return;
                    }

                    SetCraftingTutorialNodesEnabled(true);
                    UpdateTutorialDimPanels(hole, screenSize);
                    UpdateTutorialBorderPanels(hole);
                    UpdateTutorialTextPanel(hole, screenSize, GetTutorialText(focus));
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

                    heatingUiRootNode->SetEnabled(enabled);

                    if (!enabled && heatingUiText){
                        heatingUiText->text = "";
                    }
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

                void CreateDungeonPromptUi(){
                    if (!GetScene()){
                        return;
                    }

                    Font* font = LoadModelUiFont();

                    if (!font){
                        return;
                    }

                    dungeonPromptUiNode =
                        GetScene()->CreateNode("Crafting Dungeon Prompt UI Root");

                    dungeonPromptUiNode->AddObject<UiLayout>(
                        glm::ivec2(900, 190),
                        glm::ivec2(0, 0),
                        80,
                        AnchorPoint::Center
                    );

                    SceneNode* textNode =
                        GetScene()->CreateNode(
                            dungeonPromptUiNode,
                            "Crafting Dungeon Prompt UI Text"
                        );

                    textNode->AddObject<UiLayout>(
                        glm::ivec2(860, 150),
                        glm::ivec2(20, 20),
                        81,
                        AnchorPoint::TopLeft
                    );

                    dungeonPromptText =
                        textNode->AddObject<UiText>("", font);

                    dungeonPromptText->fontSize = 34.0f;
                    dungeonPromptText->color = glm::vec4(1.0f, 0.93f, 0.72f, 0.0f);
                    dungeonPromptText->alignment = TextAlignment::Middle;
                    dungeonPromptText->maxWidth = 860.0f;

                    dungeonPromptUiNode->SetEnabled(false);
                }

                void ShowDungeonPromptMessage(){
                    if (!dungeonPromptUiNode || !dungeonPromptText){
                        return;
                    }

                    dungeonPromptUiNode->SetEnabled(true);
                    dungeonPromptText->text =
                        "All that remains is to enter the dungeon\n"
                        "and use your brew.";
                    dungeonPromptText->color.w = 1.0f;

                    dungeonPromptMessageVisible = true;
                    dungeonPromptShowUntilTime = Time::Current() + 4.0f;
                }

                void HideDungeonPromptMessage(){
                    dungeonPromptMessageVisible = false;
                    dungeonPromptShowUntilTime = 0.0f;

                    if (dungeonPromptText){
                        dungeonPromptText->text = "";
                        dungeonPromptText->color.w = 0.0f;
                    }

                    if (dungeonPromptUiNode){
                        dungeonPromptUiNode->SetEnabled(false);
                    }
                }

                void UpdateDungeonPromptMessage(){
                    if (!dungeonPromptMessageVisible || !dungeonPromptText){
                        return;
                    }

                    if (Time::Current() <= dungeonPromptShowUntilTime){
                        return;
                    }

                    dungeonPromptText->color.w = glm::clamp(
                        dungeonPromptText->color.w - Time::Delta() * 1.5f,
                        0.0f,
                        1.0f
                    );

                    if (dungeonPromptText->color.w <= 0.0f){
                        HideDungeonPromptMessage();
                    }
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

                    savedCameraPosition =
                        cameraNode->GlobalTransform().Position().Value();

                    savedCameraRotation =
                        cameraNode->GlobalTransform().Rotation().Value();

                    savedCameraFov = camera->GetFov();
                    savedCameraAspectRatio = camera->GetAspectRatio();
                    savedCameraNearPlane = camera->GetNearPlane();
                    savedCameraFarPlane = camera->GetFarPlane();
                    savedCameraTransformValid = true;

                    if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                        cameraSettings->SetEnabled(false);
                    }

                    ResetCraftingSession();
                    currentStage = CraftingStage::Ingredients;

                    if (stageOneUiNode){
                        stageOneUiNode->SetEnabled(true);
                    }

                    if (stageTwoUiNode){
                        stageTwoUiNode->SetEnabled(false);
                    }

                    if (lastStageUiNode){
                        lastStageUiNode->SetEnabled(false);
                    }

                    if (stageOneCameraLightNode){
                        stageOneCameraLightNode->SetEnabled(true);
                    }

                    if (stageTwoCameraLightNode){
                        stageTwoCameraLightNode->SetEnabled(false);
                    }

                    if (lastStageCameraLightNode){
                        lastStageCameraLightNode->SetEnabled(false);
                    }

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
                    HideGameplayHudForCrafting();
                    SetIngredientsEnabled(true);
                    RefreshPlayerInventoryFromPersistent();
                    UpdateInventorySlotTexts();
                    UpdateVisibleInventoryItems();

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
                    RestoreGameplayHudAfterCrafting();
                    SetIngredientsEnabled(false);

                    if (stageOneUiNode){
                        stageOneUiNode->SetEnabled(false);
                    }

                    if (stageTwoUiNode){
                        stageTwoUiNode->SetEnabled(false);
                    }

                    if (lastStageUiNode){
                        lastStageUiNode->SetEnabled(false);
                    }

                    if (stageOneCameraLightNode){
                        stageOneCameraLightNode->SetEnabled(false);
                    }

                    if (stageTwoCameraLightNode){
                        stageTwoCameraLightNode->SetEnabled(false);
                    }

                    if (lastStageCameraLightNode){
                        lastStageCameraLightNode->SetEnabled(false);
                    }

                    Camera* camera = cameraNode->GetObject<Camera>();

                    if (savedCameraTransformValid){
                        cameraNode->GlobalTransform().Position() =
                            savedCameraPosition;

                        cameraNode->GlobalTransform().Rotation() =
                            savedCameraRotation;

                        if (camera){
                            camera->MakePerspective(
                                savedCameraFov,
                                savedCameraAspectRatio,
                                savedCameraNearPlane,
                                savedCameraFarPlane
                            );
                        }

                        savedCameraTransformValid = false;
                    }

                    if (auto* cameraSettings = cameraNode->GetObject<CameraSettings>()){
                        cameraSettings->SetEnabled(true);
                        cameraSettings->Update();
                    }

                    isActive = false;

                    GetScene()->Input()->SetMouseLocked(false);
                }
    };
}