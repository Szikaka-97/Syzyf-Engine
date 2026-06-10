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

                SceneNode* bellowNode = nullptr;
                SceneNode* bladeNode = nullptr;
                SceneNode* qualityMeterNode = nullptr;

                glm::vec3 bellowDefaultScale = glm::vec3(1.0f);
                float bellowAnimationAmount = 0.0f;

                Text3D* stageTwoQualityText = nullptr;
                Text3D* stageTwoTemperatureText = nullptr;
                std::array<SceneNode*, 5> qualityStarNodes = {};

                SceneNode* stageOneUiNode = nullptr;
                SceneNode* stageTwoUiNode = nullptr;
                SceneNode* lastStageUiNode = nullptr;

                SceneNode* stageOneCameraLightNode = nullptr;
                SceneNode* stageTwoCameraLightNode = nullptr;
                SceneNode* lastStageCameraLightNode = nullptr;

                int inventoryPage = 0;

                std::array<Crafting::IngredientData, 4> playerInventory = {};
                std::array<Text3D*, 4> inventorySlotTexts = {};
                std::array<Text3D*, 4> cauldronSlotTexts = {};

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
                        FindNodeRecursive(GetNode(), valveHitboxNodeName);

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

                    bellowNode =
                        FindNodeRecursive(GetNode(), "bellow");

                    bladeNode =
                        FindNodeRecursive(GetNode(), "blade");

                    qualityMeterNode =
                        FindNodeRecursive(GetNode(), "quality_metter");

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

                    if (bellowNode){
                        bellowDefaultScale =
                            bellowNode->LocalTransform().Scale().Value();
                    }

                    playerInventory[0] = CreateMainEffectIngredientData(
                        IngredientType::Sugar,
                        "Burn",
                        EffectId::Burn,
                        glm::vec4(1.0f, 0.1f, 0.1f, 1.0f)
                    );

                    playerInventory[1] = CreateMainEffectIngredientData(
                        IngredientType::Water,
                        "Lightning",
                        EffectId::Lightning,
                        glm::vec4(1.0f, 1.0f, 0.1f, 1.0f)
                    );

                    playerInventory[2] = CreateModifierIngredientData(
                        IngredientType::Water,
                        "Radius",
                        ModifierId::Radius,
                        1.5f,
                        glm::vec4(0.1f, 0.8f, 0.2f, 1.0f)
                    );

                    playerInventory[3] = CreateModifierIngredientData(
                        IngredientType::Sugar,
                        "Duration",
                        ModifierId::Duration,
                        2.5f,
                        glm::vec4(0.1f, 0.3f, 1.0f, 1.0f)
                    );

                    CreateStageOneModelTexts();
                    CreateStageTwoModelTexts();
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
                        cauldron->Clear();
                    }

                    UpdateCauldronSlotTexts();

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

                    if (currentStage == CraftingStage::Ingredients){
                        spdlog::info("Crafting UI Info: drag ingredients from backpack slots into the cauldron.");
                    }
                    else if (currentStage == CraftingStage::Heating){
                        spdlog::info("Crafting UI Info: click the blower to keep the temperature inside the target range.");
                    }
                    else if (currentStage == CraftingStage::Bottling){
                        spdlog::info("Crafting UI Info: click the valve while a bottle is in the fill zone.");
                    }
                }

                void OnUiNextClicked(){
                    if (!isActive){
                        return;
                    }

                    if (currentStage == CraftingStage::Ingredients){
                        OnLidClicked();
                        return;
                    }

                    if (currentStage == CraftingStage::Finished){
                        OnDoorClicked();
                    }
                }

                void OnInventoryNextPageClicked(){
                    if (!isActive || currentStage != CraftingStage::Ingredients){
                        return;
                    }

                    inventoryPage = 0;
                    UpdateInventorySlotTexts();
                }

                void OnInventoryPreviousPageClicked(){
                    if (!isActive || currentStage != CraftingStage::Ingredients){
                        return;
                    }

                    inventoryPage = 0;
                    UpdateInventorySlotTexts();
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

                    const std::array<std::string, 4> inventorySlots = {
                        "slot_bigger",
                        "slot_bigger.001",
                        "slot_bigger.002",
                        "slot_bigger.003"
                    };

                    const std::array<std::string, 4> cauldronSlots = {
                        "slot_bigger.004",
                        "slot_bigger.005",
                        "slot_bigger.006",
                        "slot_bigger.007"
                    };

                    for (int i = 0; i < 4; i++){
                        SceneNode* slotNode =
                            FindNodeRecursive(GetNode(), inventorySlots[i]);

                        inventorySlotTexts[i] = CreateModelText(
                            slotNode,
                            "InventorySlotText_" + std::to_string(i),
                            "",
                            font,
                            glm::vec3(0.0f, 0.04f, 0.06f),
                            0.055f
                        );
                    }

                    for (int i = 0; i < 4; i++){
                        SceneNode* slotNode =
                            FindNodeRecursive(GetNode(), cauldronSlots[i]);

                        cauldronSlotTexts[i] = CreateModelText(
                            slotNode,
                            "CauldronSlotText_" + std::to_string(i),
                            "Empty",
                            font,
                            glm::vec3(0.0f, 0.04f, 0.06f),
                            0.05f
                        );
                    }

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_back"),
                        "StageOneBackLabel",
                        "Back",
                        font,
                        glm::vec3(0.0f, 0.04f, 0.05f),
                        0.055f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_info"),
                        "StageOneInfoLabel",
                        "Info",
                        font,
                        glm::vec3(0.0f, 0.04f, 0.05f),
                        0.055f
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
                        glm::vec3(0.0f, 0.04f, 0.05f),
                        0.045f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_previous_page"),
                        "InventoryPreviousPageLabel",
                        "Up",
                        font,
                        glm::vec3(0.0f, 0.04f, 0.05f),
                        0.045f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_back.001"),
                        "StageTwoBackLabel",
                        "Back",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.045f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_info.001"),
                        "StageTwoInfoLabel",
                        "Info",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.045f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_next.001"),
                        "StageTwoNextLabel",
                        "Next",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.045f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_back.002"),
                        "LastStageBackLabel",
                        "Back",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.045f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_info.002"),
                        "LastStageInfoLabel",
                        "Info",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.045f
                    );

                    CreateModelText(
                        FindNodeRecursive(GetNode(), "przycisk_next.002"),
                        "LastStageNextLabel",
                        "Done",
                        font,
                        glm::vec3(0.0f, 0.03f, 0.04f),
                        0.045f
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

                void UpdateHeatingModelUi(){
                    std::ostringstream qualityText;
                    qualityText << std::fixed << std::setprecision(1);
                    qualityText << "Heating\n";
                    qualityText << "Quality: " << heatingStage.GetQualityPercent() << "%";

                    if (stageTwoQualityText){
                        stageTwoQualityText->SetText(qualityText.str());
                    }

                    std::ostringstream temperatureText;
                    temperatureText << std::fixed << std::setprecision(1);
                    temperatureText << "Temp: " << heatingStage.GetTemperature() << " C\n";
                    temperatureText << "Range: " << heatingStage.tempMin << " - " << heatingStage.tempMax << " C";

                    if (stageTwoTemperatureText){
                        stageTwoTemperatureText->SetText(temperatureText.str());
                    }

                    UpdateQualityStars();
                }

                void UpdateInventorySlotTexts(){
                    for (int i = 0; i < 4; i++){
                        if (!inventorySlotTexts[i]){
                            continue;
                        }

                        inventorySlotTexts[i]->SetText(playerInventory[i].displayName);
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

                    if (finished){
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

                    SaveCraftedPotion();

                    ExitStation();
                }

                void SaveCraftedPotion(){
                    if (!cauldron || !bottlingStage.HasEnoughFilledBottles()){
                        return;
                    }

                    CraftedPotionData potionData =
                        CraftingRecipeChecker::BuildCraftedPotion(
                            cauldron->GetIngredients(),
                            cauldron->GetQualityPercent()
                        );

                    PotionInventory::SaveLastCraftedPotion(
                        potionData,
                        bottlingStage.GetFilledBottles()
                    );

                    spdlog::error(
                        "CraftingStation: crafted '{}' effect={} secondary={} radius={:.2f} duration={:.2f} power={:.2f} bottles={}",
                        potionData.recipeName,
                        potionData.primaryEffectId,
                        potionData.secondaryEffectId,
                        potionData.radius,
                        potionData.duration,
                        potionData.power,
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

                bool FocusCameraOnBlenderCamera(const std::string& blenderCameraNodeName){
                    SceneNode* cameraNode = GetCameraNode();

                    if (!cameraNode){
                        return false;
                    }

                    SceneNode* blenderCameraRootNode =
                        FindNodeRecursive(GetNode(), blenderCameraNodeName);

                    if (!blenderCameraRootNode){
                        return false;
                    }

                    SceneNode* blenderCameraNode =
                        FindNodeRecursive(blenderCameraRootNode, "Camera");

                    if (!blenderCameraNode){
                        blenderCameraNode = blenderCameraRootNode;
                    }

                    Camera* runtimeCamera =
                        cameraNode->GetObject<Camera>();

                    Camera* blenderCamera =
                        blenderCameraNode->GetObject<Camera>();

                    cameraNode->GlobalTransform().Position() =
                        blenderCameraNode->GlobalTransform().Position().Value();

                    cameraNode->GlobalTransform().Rotation() =
                        blenderCameraNode->GlobalTransform().Rotation().Value();

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