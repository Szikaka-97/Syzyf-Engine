#pragma once

#include "Camera.h"
#include "Light.h"
#include "LightSystem.h"
#include "Material.h"
#include "Mesh.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "Shader.h"
#include "Skybox.h"
#include "Tonemapper.h"
#include <MaskEffects.h>
#include <JfaOutline.h>
#include <DepthOfField.h>
#include <Bloom.h>
#include <ColorGrading.h>
#include <Fxaa.h>
#include <GltfScene.h>

#include <TimeSystem.h>
#include <TweenSystem.h>

#include "game_scripts/CameraSettings.h"
#include "game_scripts/AimCrosshair.h"
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/ui/PauseMenu.h>
#include <ui/widgets/wheel/UiWheel.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <text/Font.h>
#include <PersistentData.h>
#include <Application.h>
#include <fog/FogVolume.h>
#include <game_scripts/PotionInventory.h>
#include "DungeonScene.h"
#include "game_scripts/crafting/CraftingDragInteractor.h"
#include "game_scripts/crafting/CraftingInteractable.h"
#include "game_scripts/crafting/DraggableCraftingItem.h"
#include "game_scripts/crafting/CraftingStation.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"
#include "game_scripts/crafting/Cauldron.h"

#include <game_scripts/PlayerController.h>
#include <physics/VirtualCharacterController.h>

#include <animation/AnimationSystem.h>
#include <ui/systems/UiSystem.h>

#include <physics/Body.h>
#include <physics/System.h>
#include <physics/Helpers.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace CraftingScene {

    static constexpr const char* CraftingRoomModelPath =
	    "./res/models/rooms/Base_final_version.glb";

    static constexpr const char* CraftingStationModelPath =
	    "./res/models/Bimbermachine.glb";

    struct IngredientSpawnData {
	    std::string nodeName;
	    std::string inventoryKey;
	    std::string modelPath;
	    glm::vec3 position = glm::vec3(0.0f);
	    glm::vec3 modelScale = glm::vec3(1.0f);
	    glm::vec3 modelRotationEuler = glm::vec3(0.0f);
	    glm::vec3 modelOffset = glm::vec3(0.0f);
	    glm::vec3 interactionHalfExtents = glm::vec3(0.07f, 0.08f, 0.07f);
	    Crafting::IngredientData ingredientData;
    };

    inline Material* CreateColorMaterial(const glm::vec4& color);

    inline MeshRenderer* FindMeshRenderer(SceneNode* node) {
	    if (!node) {
		    return nullptr;
	    }

	    return node->GetObjectInChildren<MeshRenderer>();
    }

    inline SceneNode* FindFirstNodeByNameRecursive(SceneNode* node, const std::string& name) {
	    if (!node) {
		    return nullptr;
	    }

	    if (node->GetName() == name) {
		    return node;
	    }

	    for (SceneNode* child : node->GetChildren()) {
		    SceneNode* found = FindFirstNodeByNameRecursive(child, name);

		    if (found != nullptr) {
			    return found;
		    }
	    }

	    return nullptr;
    }

    inline SceneNode* FindFirstNodeByNamesRecursive(SceneNode* node, const std::vector<std::string>& names) {
	    for (const std::string& name : names) {
		    SceneNode* foundNode = FindFirstNodeByNameRecursive(node, name);

		    if (foundNode != nullptr) {
			    return foundNode;
		    }
	    }

	    return nullptr;
    }

    inline SceneNode* FindStationMarkerNode(SceneNode* roomNode) {
	    return FindFirstNodeByNamesRecursive(
		    roomNode,
		    {
			    "CraftingStationPoint",
			    "Crafting_Station_Point",
			    "CraftingStationSpawn",
			    "Crafting_Station_Spawn",
			    "BimberMachinePoint",
			    "Bimber_Machine_Point",
			    "MachinePoint",
			    "Machine_Point",
			    "StationPoint",
			    "Station_Point",
			    "station",
			    "Station"
		    }
	    );
    }

    inline bool IsRoomPointLightNodeName(const std::string& name) {
	    return
		    name == "Point" ||
		    name.rfind("Point.", 0) == 0 ||
		    name == "PointLight" ||
		    name.rfind("PointLight.", 0) == 0;
    }

    inline void CollectPointLightsRecursive(SceneNode* node, std::vector<SceneNode*>& pointLights) {
	    if (!node) {
		    return;
	    }

	    if (IsRoomPointLightNodeName(node->GetName())) {
		    pointLights.push_back(node);
	    }

	    for (SceneNode* child : node->GetChildren()) {
		    CollectPointLightsRecursive(child, pointLights);
	    }
    }

    inline int GetPointLightIndex(const std::string& name) {
	    try {
		    if (name == "Point" || name == "PointLight") {
			    return 0;
		    }

		    const std::string pointPrefix = "Point.";
		    const std::string pointLightPrefix = "PointLight.";

		    if (name.rfind(pointPrefix, 0) == 0) {
			    return std::stoi(name.substr(pointPrefix.size()));
		    }

		    if (name.rfind(pointLightPrefix, 0) == 0) {
			    return std::stoi(name.substr(pointLightPrefix.size()));
		    }
	    }
	    catch (...) {
	    }

	    return 999999;
    }

    class CraftingRoomLights : public GameObject {
    private:
	    static constexpr const char* BaseRoomVisitedKey =
		    "Crafting_BaseRoomVisited";

	    std::vector<Light*> lights;
	    std::vector<float> baseIntensities;

	    bool firstRoomVisit = false;
	    float lightsOnTime = 0.0f;
	    float sequenceDelay = 0.25f;
	    float fadeDuration = 0.6f;

    public:
	    void Awake() {
		    std::vector<SceneNode*> pointLightNodes;

		    CollectPointLightsRecursive(GetNode(), pointLightNodes);

		    std::sort(pointLightNodes.begin(), pointLightNodes.end(), [](SceneNode* a, SceneNode* b) {
			    return GetPointLightIndex(a->GetName()) < GetPointLightIndex(b->GetName());
		    });

		    this->firstRoomVisit =
			    !PersistentData::Get<bool>(BaseRoomVisitedKey);

		    PersistentData::Set<bool>(
			    BaseRoomVisitedKey,
			    true
		    );

		    for (SceneNode* pointLightNode : pointLightNodes) {
			    Light* light = nullptr;

			    if (!pointLightNode->TryGetObject<Light>(light)) {
				    light = pointLightNode->AddObject<Light>(
					    Light::PointLight(
						    glm::vec3(1.0f, 0.55f, 0.18f),
						    10.0f,
						    0.0f,
						    0.09f,
						    0.032f
					    )
				    );
			    }

			    if (this->firstRoomVisit) {
				    light->SetIntensity(0.0f);
			    }
			    else {
				    light->SetIntensity(3.0f);
			    }

			    this->lights.push_back(light);
			    this->baseIntensities.push_back(3.0f);
		    }

		    this->lightsOnTime = Time::Current();

		    spdlog::info(
			    "CraftingScene: loaded {} room point lights. firstRoomVisit={}.",
			    this->lights.size(),
			    this->firstRoomVisit
		    );
	    }

	    void Update() {
		    for (int index = 0; index < this->lights.size(); index++) {
			    Light* light = this->lights[index];

			    float flicker =
				    glm::sin(Time::Current() * (0.6f + (light->GetID() % 4) * 0.15f) + light->GetID() * 4.0f) * 0.35f;

			    float targetIntensity =
				    this->baseIntensities[index] + flicker;

			    float turnOnAmount = 1.0f;

			    if (this->firstRoomVisit) {
				    turnOnAmount =
					    glm::clamp(
						    (Time::Current() - this->lightsOnTime - float(index) * this->sequenceDelay) / this->fadeDuration,
						    0.0f,
						    1.0f
					    );
			    }

			    light->SetIntensity(targetIntensity * turnOnAmount);
		    }
	    }
    };


    class CraftingTutorialFinishedMessage : public GameObject {
    private:
        static constexpr const char* TutorialFinishedMessageAlreadyShownKey =
            "CraftingScene_TutorialFinishedMessageAlreadyShown";

            UiText* messageText = nullptr;
            bool messageStarted = false;
            float showUntilTime = 0.0f;

	    void HideMessage() {
		    if (this->messageText == nullptr) {
			    return;
		    }

		    this->messageText->color.w = glm::clamp(
			    this->messageText->color.w - Time::Delta() * 2.0f,
			    0.0f,
			    1.0f
		    );
	    }

            void ShowMessage() {
                if (this->messageText == nullptr) {
                    return;
                }

                if (PersistentData::Get<bool>(TutorialFinishedMessageAlreadyShownKey)) {
                    PersistentData::Set<bool>(
                        PotionInventory::ShowTutorialFinishedMessageKey,
                        false
                    );

                    return;
                }

                if (!PersistentData::Get<bool>(PotionInventory::ShowTutorialFinishedMessageKey)) {
                    return;
                }

		    this->messageText->text =
			    "Tutorial complete\n"
			    "You can now brew new potions in your base.\n"
			    "Search the dungeon for new ingredients from monsters.\n"
			    "Return here after each expedition to craft stronger potions.";

		    this->messageText->color.w = 1.0f;
		    this->messageStarted = true;
		    this->showUntilTime = Time::Current() + 12.0f;

                PersistentData::Set<bool>(TutorialFinishedMessageAlreadyShownKey,true);

		    PersistentData::Set<bool>(
			    PotionInventory::ShowTutorialFinishedMessageKey,
			    false
		    );
	    }

    public:
	    void Awake() {
		    TextureParams fontTextureParams = {
			    .channels = TextureChannels::RGB,
			    .colorSpace = TextureColor::Linear,
			    .format = TextureFormat::Ubyte,
			    .wrapU = TextureWrap::Clamp,
			    .wrapV = TextureWrap::Clamp,
			    .minFilter = TextureFilter::Linear,
			    .magFilter = TextureFilter::Linear
		    };

		    Texture2D* papyrusAtlas = GetScene()->Resources()->Get<Texture2D>(
			    "./res/fonts/Papyrus/Papyrus-Regular.png",
			    fontTextureParams
		    );

		    Font* papyrusFont = GetScene()->Resources()->Get<Font>(
			    "./res/fonts/Papyrus/Papyrus-Regular.json",
			    papyrusAtlas,
			    true
		    );

		    SceneNode* uiTextNode = GetScene()->CreateNode("Crafting Tutorial Finished Message UI");
		    uiTextNode->AddObject<UiLayout>(
			    glm::uvec2(620, 220),
			    glm::ivec2(-40, 40),
			    20,
			    AnchorPoint::TopRight
		    );

		    this->messageText = uiTextNode->AddObject<UiText>("", papyrusFont);
		    this->messageText->fontSize = 25.0f;
		    this->messageText->alignment = TextAlignment::Right;
		    this->messageText->maxWidth = 580.0f;
		    this->messageText->color = glm::vec4(1.2f, 0.3f, 0.0f, 0.0f);

                ShowMessage();
	    }

	    void Update() {

		    if (!this->messageStarted) {
			    return;
		    }

		    if (Time::Current() > this->showUntilTime) {
			    HideMessage();
		    }
	    }
    };


    inline SceneNode* CreateLocalPoint(
	    Scene& scene,
	    SceneNode* parent,
	    const std::string& nodeName,
	    const glm::vec3& localPosition
    ) {
	    SceneNode* node = scene.CreateNode(parent, nodeName);
	    node->LocalTransform().Position() = localPosition;

	    return node;
    }


    inline SceneNode* CreateWorldPoint(
	    Scene& scene,
	    SceneNode* parent,
	    const std::string& nodeName,
	    const glm::vec3& worldPosition
    ) {
	    SceneNode* node = scene.CreateNode(parent, nodeName);
	    node->GlobalTransform().Position() = worldPosition;

	    return node;
    }

    inline glm::vec3 LocalPointBetweenCameraNodes(
	    SceneNode* parent,
	    const std::string& standNodeName,
	    const std::string& lookNodeName,
	    float amountBetweenStandAndLook,
	    float sideOffset,
	    float upOffset
    ) {
	    if (!parent) {
		    return glm::vec3(0.0f);
	    }

	    SceneNode* standNode =
		    FindFirstNodeByNameRecursive(parent, standNodeName);

	    SceneNode* lookNode =
		    FindFirstNodeByNameRecursive(parent, lookNodeName);

	    if (!standNode || !lookNode) {
		    return glm::vec3(0.0f);
	    }

	    const glm::vec3 worldUp =
		    glm::vec3(0.0f, 1.0f, 0.0f);

	    glm::vec3 standPosition =
		    standNode->GlobalTransform().Position().Value();

	    glm::vec3 lookPosition =
		    lookNode->GlobalTransform().Position().Value();

	    glm::vec3 forward =
		    lookPosition - standPosition;

	    if (glm::length(forward) < 0.001f) {
		    forward = glm::vec3(0.0f, 0.0f, 1.0f);
	    }

	    forward = glm::normalize(forward);

	    glm::vec3 right =
		    glm::cross(forward, worldUp);

	    if (glm::length(right) < 0.001f) {
		    right = glm::vec3(1.0f, 0.0f, 0.0f);
	    }

	    right = glm::normalize(right);

	    glm::vec3 worldPosition =
		    glm::mix(
			    standPosition,
			    lookPosition,
			    amountBetweenStandAndLook
		    ) +
		    right * sideOffset +
		    worldUp * upOffset;

	    glm::vec4 localPosition =
		    glm::inverse(parent->GlobalTransform().Value()) *
		    glm::vec4(worldPosition, 1.0f);

	    return glm::vec3(localPosition);
    }



    inline glm::vec3 WorldPointBetweenCameraNodes(
	    SceneNode* parent,
	    const std::string& standNodeName,
	    const std::string& lookNodeName,
	    float amountBetweenStandAndLook,
	    float sideOffset,
	    float upOffset
    ) {
	    if (!parent) {
		    return glm::vec3(0.0f);
	    }

	    SceneNode* standNode =
		    FindFirstNodeByNameRecursive(parent, standNodeName);

	    SceneNode* lookNode =
		    FindFirstNodeByNameRecursive(parent, lookNodeName);

	    if (!standNode || !lookNode) {
		    return glm::vec3(0.0f);
	    }

	    const glm::vec3 worldUp =
		    glm::vec3(0.0f, 1.0f, 0.0f);

	    glm::vec3 standPosition =
		    standNode->GlobalTransform().Position().Value();

	    glm::vec3 lookPosition =
		    lookNode->GlobalTransform().Position().Value();

	    glm::vec3 forward =
		    lookPosition - standPosition;

	    if (glm::length(forward) < 0.001f) {
		    forward = glm::vec3(0.0f, 0.0f, 1.0f);
	    }

	    forward = glm::normalize(forward);

	    glm::vec3 right =
		    glm::cross(forward, worldUp);

	    if (glm::length(right) < 0.001f) {
		    right = glm::vec3(1.0f, 0.0f, 0.0f);
	    }

	    right = glm::normalize(right);

	    return glm::mix(
		    standPosition,
		    lookPosition,
		    amountBetweenStandAndLook
	    ) +
	    right * sideOffset +
	    worldUp * upOffset;
    }

    inline bool StartsWith(const std::string& text, const std::string& prefix) {
	    return text.rfind(prefix, 0) == 0;
    }

    inline bool IsRoomColliderNodeName(const std::string& name) {
	    return
		    name == "Floor" ||
		    name == "floor" ||
		    name == "Walls" ||
		    StartsWith(name, "Plane") ||
		    StartsWith(name, "Cube.") ||
		    StartsWith(name, "Schody") ||
		    StartsWith(name, "Taboret") ||
		    StartsWith(name, "Stół") ||
		    StartsWith(name, "Worek") ||
		    StartsWith(name, "Szafka") ||
		    StartsWith(name, "Skrzynka") ||
		    StartsWith(name, "Skrzynia") ||
		    StartsWith(name, "beczka") ||
		    StartsWith(name, "Rura") ||
		    StartsWith(name, "pochodnia");
    }

    inline void CollectRoomColliderNodesRecursive(SceneNode* node, std::vector<SceneNode*>& colliderNodes) {
	    if (!node) {
		    return;
	    }

	    if (IsRoomColliderNodeName(node->GetName())) {
		    colliderNodes.push_back(node);
	    }

	    for (SceneNode* child : node->GetChildren()) {
		    CollectRoomColliderNodesRecursive(child, colliderNodes);
	    }
    }

    inline void AddStaticRoomCollider(SceneNode* colliderNode) {
	    if (!colliderNode) {
		    return;
	    }

	    MeshRenderer* renderer =
		    FindMeshRenderer(colliderNode);

	    if (!renderer || !renderer->GetMesh()) {
		    spdlog::warn(
			    "CraftingScene: room collider node '{}' has no mesh renderer.",
			    colliderNode->GetName()
		    );

		    return;
	    }

	    SceneNode* bodyNode =
		    renderer->GetNode();

	    if (bodyNode->GetObject<Physics::Body>()) {
		    return;
	    }

	    auto* body = bodyNode->AddObject<Physics::Body>(
		    JPH::BodyCreationSettings{
			    Physics::MeshShape(renderer->GetMesh()),
			    JPH::RVec3::sZero(),
			    JPH::Quat::sIdentity(),
			    JPH::EMotionType::Static,
			    Physics::Layers::NON_MOVING
		    }
	    );

	    body->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

	    spdlog::info(
		    "CraftingScene: added room collider to '{}'.",
		    colliderNode->GetName()
	    );
    }

    inline void AddRoomPhysics(SceneNode* roomNode) {
	    if (!roomNode) {
		    return;
	    }

	    std::vector<SceneNode*> colliderNodes;
	    CollectRoomColliderNodesRecursive(roomNode, colliderNodes);

	    for (SceneNode* colliderNode : colliderNodes) {
		    AddStaticRoomCollider(colliderNode);
	    }

	    spdlog::info(
		    "CraftingScene: room colliders requested for {} nodes.",
		    colliderNodes.size()
	    );
    }

    inline void SetInteractionBodyLayer(Physics::Body* body) {
	    if (!body) {
		    return;
	    }

	    body->SetCollisionLayerAndMask(
		    {Crafting::CraftingInteractionCollisionLayer},
		    0
	    );
    }

    inline void AddMeshPhysicsToNode(SceneNode* node) {
	    if (!node) {
		    return;
	    }

	    MeshRenderer* renderer = node->GetObject<MeshRenderer>();

	    if (!renderer || !renderer->GetMesh()) {
		    return;
	    }

	    auto* body = node->AddObject<Physics::Body>(
		    JPH::BodyCreationSettings{
			    Physics::MeshShape(renderer->GetMesh()),
			    JPH::RVec3::sZero(),
			    JPH::Quat::sIdentity(),
			    JPH::EMotionType::Static,
			    Physics::Layers::NON_MOVING
		    }
	    );

	    body->SetCollisionLayerAndMask({0}, {1});
    }

    inline void AddStationMeshPhysics(SceneNode* roomNode) {
	    if (!roomNode) {
		    return;
	    }

	    const std::vector<std::string> stationMeshNodes = {
		    "Lid",
		    "Cauldron",
		    "Reinforcement",
		    "Valve_Barrel",
		    "Knob_One",
		    "Knob_One.001",
		    "Lever",
		    "Fire_Place",
		    "Door",
		    "ramka",
		    "ramka.001",
		    "ramka.002",
		    "device",
		    "bellow",
		    "Cube"
	    };

	    for (const std::string& nodeName : stationMeshNodes) {
		    AddMeshPhysicsToNode(
			    FindFirstNodeByNameRecursive(roomNode, nodeName)
		    );
	    }
    }

    inline bool IsNodeInsideSubtree(SceneNode* node, SceneNode* subtreeRoot) {
	    SceneNode* current = node;

	    while (current) {
		    if (current == subtreeRoot) {
			    return true;
		    }

		    current = current->GetParent();
	    }

	    return false;
    }

    inline void DisableEmbeddedStationNodes(SceneNode* roomNode, SceneNode* stationNodeToKeep) {
	    if (!roomNode) {
		    return;
	    }

	    const std::vector<std::string> stationMeshNodes = {
		    "Lid",
		    "Cauldron",
		    "Reinforcement",
		    "Valve_Barrel",
		    "Knob_One",
		    "Knob_One.001",
		    "Lever",
		    "Fire_Place",
		    "Door",
		    "ramka",
		    "ramka.001",
		    "ramka.002",
		    "device"
	    };

	    for (const std::string& nodeName : stationMeshNodes) {
		    SceneNode* node = FindFirstNodeByNameRecursive(roomNode, nodeName);

		    if (node && !IsNodeInsideSubtree(node, stationNodeToKeep)) {
			    node->SetEnabled(false);
		    }
	    }
    }

    inline glm::mat4 GetNodeTransformRelativeTo(SceneNode* node, SceneNode* root) {
	    if (!node || !root) {
		    return glm::mat4(1.0f);
	    }

	    return glm::inverse(root->GlobalTransform().Value()) *
		    node->GlobalTransform().Value();
    }

    inline bool AlignStationModelToRoom(SceneNode* roomNode, SceneNode* stationNode) {
	    if (!roomNode || !stationNode) {
		    return false;
	    }

	    SceneNode* stationMarkerNode =
		    FindStationMarkerNode(roomNode);

	    if (stationMarkerNode) {
		    stationNode->GlobalTransform().Position() =
			    stationMarkerNode->GlobalTransform().Position().Value();

		    stationNode->GlobalTransform().Rotation() =
		        stationMarkerNode->GlobalTransform().Rotation().Value() + glm::quat(glm::radians(glm::vec3(0.0f, 360.0f, 0.0f)));

		    stationNode->GlobalTransform().Scale() =
			    stationMarkerNode->GlobalTransform().Scale().Value();

		    return true;
	    }

	    SceneNode* roomCauldronNode =
		    FindFirstNodeByNameRecursive(roomNode, "Cauldron");

	    SceneNode* stationCauldronNode =
		    FindFirstNodeByNameRecursive(stationNode, "Cauldron");

	    if (!roomCauldronNode || !stationCauldronNode) {
		    spdlog::error(
			    "CraftingScene: missing crafting machine marker and cannot fallback to Cauldron alignment."
		    );

		    return false;
	    }

	    glm::mat4 roomGlobalTransform =
		    roomNode->GlobalTransform().Value();

	    glm::mat4 roomCauldronGlobalTransform =
		    roomCauldronNode->GlobalTransform().Value();

	    glm::mat4 stationCauldronRelativeTransform =
		    GetNodeTransformRelativeTo(stationCauldronNode, stationNode);

	    glm::mat4 stationGlobalTransform =
		    roomCauldronGlobalTransform *
		    glm::inverse(stationCauldronRelativeTransform);

	    glm::mat4 stationLocalTransform =
		    glm::inverse(roomGlobalTransform) *
		    stationGlobalTransform;

	    stationNode->LocalTransform() = stationLocalTransform;

	    return true;
    }

    inline SceneNode* CreateCraftingStationModel(Scene& scene, SceneNode* roomNode) {
	    if (!roomNode) {
		    return nullptr;
	    }

	    GltfScene* stationModel =
		    ResourceDatabase::Global->Get<GltfScene>(
			    CraftingStationModelPath
		    );

	    if (!stationModel) {
		    spdlog::error(
			    "CraftingScene: cannot load crafting machine model '{}'.",
			    CraftingStationModelPath
		    );

		    return nullptr;
	    }

	    SceneNode* stationNode =
		    stationModel->Instantiate(&scene, roomNode, "Crafting Station");

	    if (!stationNode) {
		    return nullptr;
	    }

	    AlignStationModelToRoom(roomNode, stationNode);

	    return stationNode;
    }

    inline SceneNode* CreateStationHitbox(Scene& scene, SceneNode* roomNode) {
	    if (!roomNode) {
		    return nullptr;
	    }

	    SceneNode* stationHitboxNode =
		    scene.CreateNode(roomNode, "StationHitbox");

	    SceneNode* stationCenterNode =
		    FindFirstNodeByNameRecursive(roomNode, "Cauldron");

	    if (!stationCenterNode) {
		    stationCenterNode =
			    FindFirstNodeByNameRecursive(roomNode, "Fire_Place");
	    }

	    if (!stationCenterNode) {
		    stationCenterNode =
			    FindFirstNodeByNameRecursive(roomNode, "Door");
	    }

	    if (stationCenterNode) {
		    glm::vec3 position =
			    stationCenterNode->GlobalTransform().Position().Value();

		    position.y =
			    roomNode->GlobalTransform().Position().Value().y + 1.2f;

		    stationHitboxNode->GlobalTransform().Position() =
			    position;
	    }
	    else {
		    stationHitboxNode->LocalTransform().Position() =
			    glm::vec3(0.0f, 1.2f, 0.0f);
	    }

	    stationHitboxNode->LocalTransform().Scale() =
		    glm::vec3(1.0f);

	    return stationHitboxNode;
    }

    inline SceneNode* CreateBlowerHitbox(Scene& scene, SceneNode* roomNode) {
	    if (!roomNode) {
		    return nullptr;
	    }

	    SceneNode* blowerNode =
		    FindFirstNodeByNameRecursive(roomNode, "bellow");

	    if (!blowerNode) {
		    blowerNode = FindFirstNodeByNameRecursive(roomNode, "Fire_Place");
	    }

	    if (!blowerNode) {
		    blowerNode = roomNode;
	    }

	    SceneNode* blowerHitboxNode =
		    scene.CreateNode(blowerNode, "BlowerHitbox");

	    blowerHitboxNode->LocalTransform().Position() =
		    glm::vec3(0.0f);

	    blowerHitboxNode->LocalTransform().Scale() =
		    glm::vec3(0.45f);

	    glm::vec3 blowerHitboxPosition =
		    blowerHitboxNode->GlobalTransform().Position().Value();

	    auto* blowerBody = blowerHitboxNode->AddObject<Physics::Body>(
		    JPH::BodyCreationSettings{
			    Physics::BoxShape(glm::vec3(1.0f)),
			    JPH::RVec3(
				    blowerHitboxPosition.x,
				    blowerHitboxPosition.y,
				    blowerHitboxPosition.z
			    ),
			    JPH::Quat::sIdentity(),
			    JPH::EMotionType::Static,
			    Physics::Layers::NON_MOVING
		    }
	    );

	    blowerBody->SetPosition(blowerHitboxPosition);
	    blowerBody->SetIsSensor(true);
	    SetInteractionBodyLayer(blowerBody);

	    auto* blowerInteractable =
		    blowerHitboxNode->AddObject<Crafting::CraftingInteractable>();

	    blowerInteractable->type = Crafting::CraftingInteractionType::Blower;
	    blowerInteractable->interactionEnabled = false;
	    blowerInteractable->blinkOutline = true;

	    return blowerHitboxNode;
    }

    inline SceneNode* CreateValveHitbox(Scene& scene, SceneNode* roomNode) {
	    if (!roomNode) {
		    return nullptr;
	    }

	    SceneNode* valveNode =
		    FindFirstNodeByNameRecursive(roomNode, "Knob_One.001");

	    if (!valveNode) {
		    valveNode = FindFirstNodeByNameRecursive(roomNode, "Knob_One");
	    }

	    if (!valveNode) {
		    valveNode = FindFirstNodeByNameRecursive(roomNode, "Valve_Barrel");
	    }

	    if (!valveNode) {
		    return nullptr;
	    }

	    MeshRenderer* renderer =
		    valveNode->GetObject<MeshRenderer>();

	    if (renderer && renderer->GetMesh() && !valveNode->GetObject<Physics::Body>()) {
		    auto* valveBody = valveNode->AddObject<Physics::Body>(
			    JPH::BodyCreationSettings{
				    Physics::MeshShape(renderer->GetMesh()),
				    JPH::RVec3::sZero(),
				    JPH::Quat::sIdentity(),
				    JPH::EMotionType::Static,
				    Physics::Layers::NON_MOVING
			    }
		    );

		    valveBody->SetIsSensor(true);
		    SetInteractionBodyLayer(valveBody);
		    valveBody->SetPosition(valveNode->GlobalTransform().Position().Value());
		    valveBody->SetRotation(valveNode->GlobalTransform().Rotation().Value());
	    }
	    else if (auto* valveBody = valveNode->GetObject<Physics::Body>()) {
		    valveBody->SetIsSensor(true);
		    SetInteractionBodyLayer(valveBody);
		    valveBody->SetPosition(valveNode->GlobalTransform().Position().Value());
		    valveBody->SetRotation(valveNode->GlobalTransform().Rotation().Value());
	    }

	    auto* valveInteractable =
		    valveNode->GetObject<Crafting::CraftingInteractable>();

	    if (!valveInteractable) {
		    valveInteractable = valveNode->AddObject<Crafting::CraftingInteractable>();
	    }

	    valveInteractable->type = Crafting::CraftingInteractionType::Valve;
	    valveInteractable->interactionEnabled = false;
	    valveInteractable->blinkOutline = true;

	    return valveNode;
    }

    inline glm::vec3 GetUiButtonLocalHitboxHalfExtents(SceneNode* node) {
	    glm::vec3 wantedWorldHalfExtents =
		    glm::vec3(0.28f, 0.16f, 0.08f);

	    if (!node) {
		    return wantedWorldHalfExtents;
	    }

	    glm::vec3 globalScale =
		    node->GlobalTransform().Scale().Value();

	    globalScale.x = std::max(std::abs(globalScale.x), 0.001f);
	    globalScale.y = std::max(std::abs(globalScale.y), 0.001f);
	    globalScale.z = std::max(std::abs(globalScale.z), 0.001f);

	    return glm::vec3(
		    wantedWorldHalfExtents.x / globalScale.x,
		    wantedWorldHalfExtents.y / globalScale.y,
		    wantedWorldHalfExtents.z / globalScale.z
	    );
    }

    inline void CreateUiButtonInteractable(
	    SceneNode* rootNode,
	    const std::string& nodeName,
	    Crafting::CraftingInteractionType interactionType
    ) {
	    SceneNode* node =
		    FindFirstNodeByNameRecursive(rootNode, nodeName);

	    if (!node) {
		    return;
	    }

	    auto* body =
		    node->GetObject<Physics::Body>();

	    if (!body) {
		    body = node->AddObject<Physics::Body>(
			    JPH::BodyCreationSettings{
				    Physics::BoxShape(GetUiButtonLocalHitboxHalfExtents(node)),
				    JPH::RVec3::sZero(),
				    JPH::Quat::sIdentity(),
				    JPH::EMotionType::Static,
				    Physics::Layers::NON_MOVING
			    }
		    );
	    }

	    body->SetIsSensor(true);
	    SetInteractionBodyLayer(body);
	    body->SetPosition(node->GlobalTransform().Position().Value());
	    body->SetRotation(node->GlobalTransform().Rotation().Value());

	    auto* interactable =
		    node->GetObject<Crafting::CraftingInteractable>();

	    if (!interactable) {
		    interactable = node->AddObject<Crafting::CraftingInteractable>();
	    }

	    interactable->type = interactionType;
	    interactable->interactionEnabled = true;
    }

    inline void CreateCraftingUiButtonInteractables(SceneNode* stationNode) {
	    if (!stationNode) {
		    return;
	    }

	    CreateUiButtonInteractable(stationNode, "przycisk_back", Crafting::CraftingInteractionType::UiBack);
	    CreateUiButtonInteractable(stationNode, "przycisk_info", Crafting::CraftingInteractionType::UiInfo);
	    CreateUiButtonInteractable(stationNode, "przycisk_next", Crafting::CraftingInteractionType::UiNext);
	    CreateUiButtonInteractable(stationNode, "przycisk_next_page", Crafting::CraftingInteractionType::InventoryNextPage);
	    CreateUiButtonInteractable(stationNode, "przycisk_previous_page", Crafting::CraftingInteractionType::InventoryPreviousPage);

	    CreateUiButtonInteractable(stationNode, "przycisk_back.001", Crafting::CraftingInteractionType::UiBack);
	    CreateUiButtonInteractable(stationNode, "przycisk_info.001", Crafting::CraftingInteractionType::UiInfo);
	    CreateUiButtonInteractable(stationNode, "przycisk_next.001", Crafting::CraftingInteractionType::UiNext);

	    CreateUiButtonInteractable(stationNode, "przycisk_back.002", Crafting::CraftingInteractionType::UiBack);
	    CreateUiButtonInteractable(stationNode, "przycisk_info.002", Crafting::CraftingInteractionType::UiInfo);
	    CreateUiButtonInteractable(stationNode, "przycisk_next.002", Crafting::CraftingInteractionType::UiNext);
    }

    inline glm::vec3 GetCraftingSlotWorldPosition(
	    SceneNode* roomNode,
	    const std::string& slotNodeName,
	    const glm::vec3& fallbackPosition
    ) {
	    SceneNode* slotNode =
		    FindFirstNodeByNameRecursive(roomNode, slotNodeName);

	    if (!slotNode) {
		    return fallbackPosition;
	    }

	    return slotNode->GlobalTransform().Position().Value();
    }

    inline Material* CreateColorMaterial(const glm::vec4& color) {
	    ShaderProgram* shader =
		    ShaderProgram::Build()
			    .WithVertexShader("./res/shaders/lit.vert")
			    .WithPixelShader("./res/shaders/lambert color.frag")
			    .Link();

	    Material* material = new Material(shader);
	    material->SetValue("uColor", glm::vec3(color));
	    material->SetValue("specularValue", 0.0f);

	    return material;
    }

    inline SceneNode* CreateBottlingDebugCube(
	    Scene& scene,
	    SceneNode* parent,
	    const std::string& nodeName,
	    Mesh* mesh,
	    Material* material,
	    const glm::vec3& worldPosition,
	    const glm::vec3& localScale
    ) {
	    SceneNode* node = scene.CreateNode(parent, nodeName);

	    node->LocalTransform().Scale() = localScale;
	    node->GlobalTransform().Position() = worldPosition;

	    if (mesh && material) {
		    node->AddObject<MeshRenderer>(mesh, material);
	    }

	    return node;
    }


    inline SceneNode* CreateBottlingLocalCube(
	    Scene& scene,
	    SceneNode* parent,
	    const std::string& nodeName,
	    Mesh* mesh,
	    Material* material,
	    const glm::vec3& localPosition,
	    const glm::vec3& localScale
    ) {
	    SceneNode* node = scene.CreateNode(parent, nodeName);

	    node->LocalTransform().Position() = localPosition;
	    node->LocalTransform().Scale() = localScale;

	    if (mesh && material) {
		    node->AddObject<MeshRenderer>(mesh, material);
	    }

	    return node;
    }

    inline void CreateBottlingStageNodes(Scene& scene, SceneNode* roomNode) {
	    if (!roomNode) {
		    return;
	    }

	    SceneNode* bottleStartNode =
		    FindFirstNodeByNameRecursive(roomNode, "Bottle_Start");

	    SceneNode* bottleStopNode =
		    FindFirstNodeByNameRecursive(roomNode, "Bottle_stop");

	    SceneNode* pourButtonNode =
		    FindFirstNodeByNameRecursive(roomNode, "Knob_One.001");

	    if (!pourButtonNode) {
		    pourButtonNode =
			    FindFirstNodeByNameRecursive(roomNode, "Knob_One");
	    }

	    if (!bottleStartNode || !bottleStopNode || !pourButtonNode) {
		    return;
	    }

	    const glm::mat4 bottleStartTransform =
		    GetNodeTransformRelativeTo(
			    bottleStartNode,
			    roomNode
		    );

	    const glm::mat4 bottleStopTransform =
		    GetNodeTransformRelativeTo(
			    bottleStopNode,
			    roomNode
		    );

	    const glm::mat4 pourButtonTransform =
		    GetNodeTransformRelativeTo(
			    pourButtonNode,
			    roomNode
		    );

	    const glm::vec3 bottleStartPoint =
		    glm::vec3(
			    bottleStartTransform[3]
		    );

	    const glm::vec3 bottleEndPoint =
		    glm::vec3(
			    bottleStopTransform[3]
		    );

	    const glm::vec3 pourButtonPoint =
		    glm::vec3(
			    pourButtonTransform[3]
		    );

	    const glm::vec3 path =
		    bottleEndPoint - bottleStartPoint;

	    float fillT = 0.5f;

	    const float pathLengthSquared =
		    glm::dot(path,path);

	    if (pathLengthSquared > 0.0001f) {
		    fillT =
			    glm::clamp(
				    glm::dot(pourButtonPoint - bottleStartPoint,path) /
				    pathLengthSquared,
				    0.0f,
				    1.0f
			    );
	    }

	    const glm::vec3 bottleFillPoint =
		    glm::mix(
			    bottleStartPoint,
			    bottleEndPoint,
			    fillT
		    );


	    const glm::vec3 bottleScale =
		    glm::vec3(0.16f, 0.36f, 0.16f);


	    Mesh* cubeMesh =
		    scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

	    Material* bottleMaterial =
		    CreateColorMaterial(glm::vec4(0.35f, 0.75f, 1.0f, 0.9f));

	    Material* liquidMaterial =
		    CreateColorMaterial(glm::vec4(0.9f, 0.25f, 0.15f, 0.95f));


	    SceneNode* bottlesRoot =
		    scene.CreateNode(roomNode, "BottlingBottlesRoot");

	    bottlesRoot->LocalTransform().Position() =
		    glm::vec3(0.0f);


	    for (int i = 0; i < 4; ++i) {
		    SceneNode* bottleNode = CreateBottlingLocalCube(
			    scene,
			    bottlesRoot,
			    "BottlingBottle_0" + std::to_string(i + 1),
			    cubeMesh,
			    bottleMaterial,
			    bottleStartPoint,
			    bottleScale
		    );

		    SceneNode* liquidNode = CreateBottlingLocalCube(
			    scene,
			    bottleNode,
			    "BottlingBottle_0" + std::to_string(i + 1) + "_Liquid",
			    cubeMesh,
			    liquidMaterial,
			    glm::vec3(0.0f, -0.08f, 0.0f),
			    glm::vec3(0.11f, 0.23f, 0.11f)
		    );

		    liquidNode->SetEnabled(false);
	    }

	    bottlesRoot->SetEnabled(false);
    }

    inline Crafting::IngredientData CreateMainEffectIngredient(
	    Crafting::IngredientType ingredientType,
	    const std::string& displayName,
	    const std::string& effectId,
	    const glm::vec4& color
    ) {
	    Crafting::IngredientData data;

	    data.type = ingredientType;
	    data.displayName = displayName;
	    data.role = Crafting::IngredientRole::MainEffect;
	    data.effectId = effectId;
	    data.modifierId = Crafting::ModifierId::None;
	    data.value = 1.0f;
	    data.color = color;

	    return data;
    }

    inline Crafting::IngredientData CreateModifierIngredient(
	    Crafting::IngredientType ingredientType,
	    const std::string& displayName,
	    const std::string& modifierId,
	    float value,
	    const glm::vec4& color
    ) {
	    Crafting::IngredientData data;

	    data.type = ingredientType;
	    data.displayName = displayName;
	    data.role = Crafting::IngredientRole::Modifier;
	    data.effectId = Crafting::EffectId::None;
	    data.modifierId = modifierId;
	    data.value = value;
	    data.color = color;

	    return data;
    }



    class DungeonEntryPrompt : public GameObject {
    private:
        SceneNode* dungeonEntryNode = nullptr;
        UiLayout* promptLayout = nullptr;
        UiLayout* dialogLayout = nullptr;
        UiText* promptText = nullptr;
        UiText* potionListText = nullptr;
        UiInteractable* enterButton = nullptr;
        UiInteractable* backButton = nullptr;
        bool dialogVisible = false;
        bool sceneRequested = false;
        float interactionRadius = 2.6f;

        UiLayout* ConfigureLayout(
            SceneNode* node,
            const glm::uvec2& size,
            const glm::ivec2& offset,
            int zIndex,
            AnchorPoint anchorPoint
        ){
            UiLayout* layout = node->AddObjectIfMissing<UiLayout>();
            layout->size = glm::ivec2(size);
            layout->offset = offset;
            layout->zIndex = zIndex;
            layout->anchorPoint = anchorPoint;
            return layout;
        }

        UiVisual* ConfigureVisual(
            SceneNode* node,
            const glm::vec4& color,
            const glm::vec4& hoverColor = glm::vec4(-1.0f)
        ){
            UiVisual* visual = node->AddObjectIfMissing<UiVisual>();
            visual->color = color;
            visual->colorHovered = hoverColor.x >= 0.0f
                ? std::optional<glm::vec4>(hoverColor)
                : std::nullopt;
            return visual;
        }

        UiText* CreateText(
            SceneNode* parent,
            const std::string& nodeName,
            const std::string& text,
            Font* font,
            const glm::uvec2& size,
            const glm::ivec2& offset,
            float fontSize,
            int zIndex
        ){
            SceneNode* textNode = GetScene()->GetOrCreateNode(parent,nodeName);
            ConfigureLayout(textNode,size,offset,zIndex,AnchorPoint::Center);

            UiText* uiText = textNode->AddObjectIfMissing<UiText>();
            uiText->text = text;
            uiText->font = font;
            uiText->fontSize = fontSize;
            uiText->alignment = TextAlignment::Middle;
            uiText->verticalAlignment = TextVerticalAlignment::Middle;
            uiText->color = glm::vec4(1.0f);
            return uiText;
        }

        UiInteractable* CreateButton(
            SceneNode* parent,
            const std::string& nodeName,
            const std::string& text,
            Font* font,
            const glm::uvec2& size,
            const glm::ivec2& offset,
            const glm::vec4& color = glm::vec4(0.4f,0.4f,0.4f,1.0f),
            const glm::vec4& hoverColor = glm::vec4(1.0f,1.0f,1.0f,1.0f)
        ){
            SceneNode* buttonNode = GetScene()->GetOrCreateNode(parent,nodeName);
            ConfigureLayout(buttonNode,size,offset,132,AnchorPoint::Center);
            ConfigureVisual(buttonNode,color,hoverColor);

            UiInteractable* interactable = buttonNode->AddObjectIfMissing<UiInteractable>();
            interactable->isInteractable = true;

            CreateText(
                buttonNode,
                nodeName + " Text",
                text,
                font,
                size,
                glm::ivec2(0,0),
                20.0f,
                133
            );

            return interactable;
        }

        std::string PotionLine(
            const Crafting::CraftedPotionData& potionData,
            int count
        ) const{
            std::string label = potionData.primaryEffectId;

            if (!potionData.secondaryEffectId.empty() &&
                potionData.secondaryEffectId != Crafting::EffectId::None){
                label += " + " + potionData.secondaryEffectId;
            }

            if (label.empty()){
                label = potionData.recipeName.empty() ? "Potion" : potionData.recipeName;
            }

            return "x" + std::to_string(count) + "  " + label;
        }

        std::string BuildPotionInventoryText() const{
            std::vector<PotionInventory::PotionInventoryEntry> potions =
                PotionInventory::GetPotionInventory();

            if (potions.empty()){
                int legacyPotionCount = PotionInventory::GetPotionCount();

                if (legacyPotionCount > 0){
                    return PotionLine(
                        PotionInventory::GetLastCraftedPotion(),
                        legacyPotionCount
                    );
                }

                return "No potions";
            }

            std::stringstream stream;

            for (std::size_t i = 0; i < potions.size(); i++){
                stream << PotionLine(potions[i].data,potions[i].count);

                if (i + 1 < potions.size()){
                    stream << "\n";
                }
            }

            return stream.str();
        }

        void SetPromptVisible(bool visible){
            if (this->promptLayout){
                this->promptLayout->offset = visible
                    ? glm::ivec2(-40,270)
                    : glm::ivec2(9999,9999);
            }
        }

        void SetDialogVisible(bool visible){
            this->dialogVisible = visible;

            if (visible && this->potionListText){
                this->potionListText->text = BuildPotionInventoryText();
            }

            if (this->dialogLayout){
                this->dialogLayout->offset = visible
                    ? glm::ivec2(0,0)
                    : glm::ivec2(9999,9999);
            }
        }

        bool IsPlayerNearDungeonEntry() const {
            if (!this->dungeonEntryNode || PlayerController::Instance() == nullptr) {
                return false;
            }

            return glm::distance(
                PlayerController::Instance()->GlobalTransform().Position().Value(),
                this->dungeonEntryNode->GlobalTransform().Position().Value()
            ) <= this->interactionRadius;
        }

        void EnterDungeon() {
            if (this->sceneRequested) {
                return;
            }

            this->sceneRequested = true;
            SetDialogVisible(false);
            SetPromptVisible(false);

            PersistentData::Set<bool>("CraftingScene_AutoEnterCrafting", false);
            PersistentData::Set<bool>("CraftingScene_ReturnedFromThrowingTutorial", false);

            Application::Get()->RequestSceneBuild(
                [](Scene* s) { DungeonScene::InitScene(*s); }
            );
        }

    public:
        void Awake() {
            this->dungeonEntryNode =
                FindFirstNodeByNamesRecursive(
                    GetNode(),
                    {
                        "DungeonEntry",
                        "Dungeon Entry",
                        "Dungeon_Entry",
                        "DungeonEntryPoint",
                        "Dungeon_Entry_Point"
                    }
                );

            if (!this->dungeonEntryNode) {
                spdlog::warn("CraftingScene: DungeonEntry node not found. Dungeon prompt disabled.");
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

            Texture2D* fontAtlas = GetScene()->Resources()->Get<Texture2D>(
                "./res/fonts/OpenSans-Regular/OpenSans-Regular.png",
                fontTextureParams
            );

            Font* font = GetScene()->Resources()->Get<Font>(
                "./res/fonts/OpenSans-Regular/OpenSans-Regular.json",
                fontAtlas
            );

            SceneNode* promptNode = GetScene()->GetOrCreateNode("Dungeon Entry Prompt UI");
            this->promptLayout = ConfigureLayout(
                promptNode,
                glm::uvec2(520,150),
                glm::ivec2(9999,9999),
                120,
                AnchorPoint::TopRight
            );

            this->promptText = promptNode->AddObjectIfMissing<UiText>();
            this->promptText->text = "Dungeon entrance\nPress F to inspect";
            this->promptText->font = font;
            this->promptText->fontSize = 24.0f;
            this->promptText->alignment = TextAlignment::Right;
            this->promptText->verticalAlignment = TextVerticalAlignment::Middle;
            this->promptText->maxWidth = 480.0f;
            this->promptText->color = glm::vec4(1.0f);

            SceneNode* dialogNode = GetScene()->GetOrCreateNode("Dungeon Entry Menu");
            this->dialogLayout = ConfigureLayout(
                dialogNode,
                glm::uvec2(560,460),
                glm::ivec2(9999,9999),
                130,
                AnchorPoint::Center
            );
            ConfigureVisual(dialogNode,glm::vec4(0.08f,0.08f,0.08f,0.98f));
            dialogNode->AddObjectIfMissing<UiInteractable>();

            CreateText(
                dialogNode,
                "Dungeon Entry Title",
                "Dungeon entry",
                font,
                glm::uvec2(480,44),
                glm::ivec2(0,-185),
                28.0f,
                132
            );

            CreateText(
                dialogNode,
                "Dungeon Entry Question",
                "Enter dungeon with these potions?",
                font,
                glm::uvec2(480,40),
                glm::ivec2(0,-130),
                20.0f,
                132
            );

            CreateText(
                dialogNode,
                "Dungeon Entry Potions Label",
                "Potions:",
                font,
                glm::uvec2(460,30),
                glm::ivec2(0,-80),
                18.0f,
                132
            );

            this->potionListText = CreateText(
                dialogNode,
                "Dungeon Entry Potions Text",
                "No potions",
                font,
                glm::uvec2(460,150),
                glm::ivec2(0,10),
                18.0f,
                132
            );
            this->potionListText->maxWidth = 430.0f;

            this->enterButton = CreateButton(
                dialogNode,
                "Dungeon Entry Confirm Button",
                "Enter dungeon",
                font,
                glm::uvec2(200,44),
                glm::ivec2(-115,165),
                glm::vec4(0.4f,0.4f,0.4f,1.0f),
                glm::vec4(1.0f)
            );

            this->backButton = CreateButton(
                dialogNode,
                "Dungeon Entry Back Button",
                "Back",
                font,
                glm::uvec2(200,44),
                glm::ivec2(115,165),
                glm::vec4(0.8f,0.2f,0.2f,1.0f),
                glm::vec4(1.0f)
            );

            SetDialogVisible(false);
            SetPromptVisible(false);
        }

        void Update() {
            if (this->sceneRequested || !this->dungeonEntryNode) {
                return;
            }

            bool playerNear = IsPlayerNearDungeonEntry();

            if (!playerNear) {
                SetDialogVisible(false);
                SetPromptVisible(false);
                return;
            }

            if (this->dialogVisible) {
                SetPromptVisible(false);

                if (this->enterButton && this->enterButton->isDown) {
                    EnterDungeon();
                    return;
                }

                if ((this->backButton && this->backButton->isDown) ||
                    GetScene()->Input()->KeyDown(Key::N) ||
                    GetScene()->Input()->KeyDown(Key::Escape)) {
                    SetDialogVisible(false);
                    return;
                }

                if (GetScene()->Input()->KeyDown(Key::Y) ||
                    GetScene()->Input()->KeyDown(Key::Enter) ||
                    GetScene()->Input()->KeyDown(Key::NumpadEnter)) {
                    EnterDungeon();
                    return;
                }

                return;
            }

            SetPromptVisible(true);

            if (GetScene()->Input()->KeyDown(Key::F)) {
                SetDialogVisible(true);
            }
        }
    };


    inline void HideMeshRenderersRecursive(SceneNode* node) {
        if (!node) {
            return;
        }

        if (auto* renderer = node->GetObject<MeshRenderer>()) {
            renderer->SetEnabled(false);
        }

        for (SceneNode* child : node->GetChildren()) {
            HideMeshRenderersRecursive(child);
        }
    }

    inline void CreateFogVolumeAtFogPoint(Scene& scene, SceneNode* roomNode) {
        if (!roomNode) {
            return;
        }

        SceneNode* dungeonEntryNode =
            FindFirstNodeByNamesRecursive(
                roomNode,
                {
                    "DungeonEntry",
                    "Dungeon Entry",
                    "Dungeon_Entry",
                    "DungeonEntryPoint",
                    "Dungeon_Entry_Point"
                }
            );

        SceneNode* fogPointNode =
            FindFirstNodeByNamesRecursive(
                roomNode,
                {
                    "Fog",
                    "fog",
                    "FogPoint",
                    "Fog_Point",
                    "Fog Volume",
                    "FogVolume"
                }
            );

        SceneNode* fogSourceNode = dungeonEntryNode;

        if (!fogSourceNode) {
            fogSourceNode = fogPointNode;
        }

        if (!fogSourceNode) {
            spdlog::warn("CraftingScene: DungeonEntry/Fog point not found. Fog volume was not created.");
            return;
        }

        if (dungeonEntryNode) {
            HideMeshRenderersRecursive(dungeonEntryNode);
        }

        SceneNode* fogNode =
            scene.CreateNode(roomNode, "Dungeon Entry Fog Volume");

        fogNode->GlobalTransform().Position() =
            fogSourceNode->GlobalTransform().Position().Value();

        glm::vec3 fogScale =
            fogSourceNode->GlobalTransform().Scale().Value();

        if (glm::length(fogScale) < 0.01f) {
            fogScale = glm::vec3(8.0f, 3.0f, 8.0f);
        }

        fogScale *= glm::vec3(1.9f, 1.45f, 1.9f);

        fogNode->GlobalTransform().Scale() = fogScale;

        auto* fogVolume = fogNode->AddObject<FogVolume>();
        fogVolume->stepSize = 0.045f;
        fogVolume->scatteringDensity = 1.35f;
        fogVolume->absorptionDensity = 0.18f;
        fogVolume->scatteringColor = glm::vec3(0.8f, 0.72f, 0.58f);
        fogVolume->coverage = 0.92f;
        fogVolume->sharpness = 3.5f;
    }

    inline Crafting::DraggableCraftingItem* CreateDraggableIngredientModel(
	    Scene& scene,
	    SceneNode* parent,
	    const IngredientSpawnData& spawn
    ) {
	    SceneNode* node = scene.CreateNode(parent, spawn.nodeName);
	    node->GlobalTransform().Position() = spawn.position;

	    GltfScene* ingredientModel =
		    ResourceDatabase::Global->Get<GltfScene>(
			    spawn.modelPath
		    );

	    if (!ingredientModel) {
		    spdlog::error(
			    "CraftingScene: cannot load ingredient model '{}'.",
			    spawn.modelPath
		    );

		    return nullptr;
	    }

	    SceneNode* modelNode =
		    ingredientModel->Instantiate(
			    &scene,
			    node,
			    spawn.nodeName + " Model"
		    );

	    if (!modelNode) {
		    spdlog::error(
			    "CraftingScene: failed to instantiate ingredient model '{}'.",
			    spawn.modelPath
		    );

		    return nullptr;
	    }

	    modelNode->LocalTransform().Position() =
		    spawn.modelOffset;

	    modelNode->LocalTransform().Scale() =
		    spawn.modelScale;

	    modelNode->LocalTransform().Rotation() =
		    glm::quat(glm::radians(spawn.modelRotationEuler));

	    auto* item = node->AddObject<Crafting::DraggableCraftingItem>();
	    item->inventoryKey = spawn.inventoryKey;
	    item->data = spawn.ingredientData;
	    item->data.inventoryKey = spawn.inventoryKey;

	    auto* interactable = node->AddObject<Crafting::CraftingInteractable>();
	    interactable->type = Crafting::CraftingInteractionType::Ingredient;
	    interactable->interactionEnabled = true;
	    interactable->blinkOutline = false;

	    glm::vec3 globalPosition =
		    node->GlobalTransform().Position().Value();

	    auto* body = node->AddObject<Physics::Body>(
		    JPH::BodyCreationSettings{
			    Physics::BoxShape(spawn.interactionHalfExtents),
			    JPH::RVec3(globalPosition.x, globalPosition.y, globalPosition.z),
			    JPH::Quat::sIdentity(),
			    JPH::EMotionType::Kinematic,
			    Physics::Layers::MOVING
		    }
	    );

	    body->SetGravityFactor(0.0f);
	    body->SetLinearVelocity(glm::vec3(0.0f));
	    body->SetAngularVelocity(glm::vec3(0.0f));
	    body->SetPosition(globalPosition);
	    body->SetIsSensor(true);
	    SetInteractionBodyLayer(body);

	    return item;
    }
    inline SceneNode* CreatePlayer(Scene& scene, SceneNode* roomNode) {
	    JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
		    new JPH::CharacterVirtualSettings();

	    characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
	    characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
	    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

	    SceneNode* playerNode = scene.CreateNode("Player");

	    SceneNode* bimberman = ResourceDatabase::Global->Get<GltfScene>(
		    "./res/models/bimbermann_throwing.glb"
	    )->Instantiate(&scene, scene.root, "Bimberman");

	    if (bimberman) {
		    bimberman->SetParent(playerNode);
	    }

	    SceneNode* aimReticle =
		    ResourceDatabase::Global->Get<GltfScene>(
			    "./res/models/crosshair.glb"
		    )->Instantiate(&scene, playerNode, "Aim Reticle");

	    aimReticle->AddObject<AimCrosshair>();
	    aimReticle->SetEnabled(false);

	    bool returnedFromThrowingTutorial =
		    PersistentData::Get<bool>("CraftingScene_ReturnedFromThrowingTutorial");

	    SceneNode* spawnNode = nullptr;

	    if (returnedFromThrowingTutorial) {
		    spawnNode =
			    FindFirstNodeByNamesRecursive(
				    roomNode,
				    {
					    "PlayerReturnSpawn",
					    "Player_Return_Spawn",
					    "ThrowingTutorialReturnSpawn",
					    "Throwing_Tutorial_Return_Spawn",
					    "DungeonEntrySpawn",
					    "Dungeon_Entry_Spawn",
					    "DungeonEntry",
					    "Dungeon Entry"
				    }
			    );
	    }

	    if (!spawnNode) {
		    spawnNode =
			    FindFirstNodeByNamesRecursive(
				    roomNode,
				    {
					    "Spawn",
				    }
			    );
	    }

	    if (spawnNode) {
		    playerNode->GlobalTransform().Position() =
			    spawnNode->GlobalTransform().Position().Value();
	    }
	    else {
		    playerNode->GlobalTransform().Position() =
			    glm::vec3(4.0f, 0.0f, -1.0f);
	    }

	    auto* virtualCharacter =
		    playerNode->AddObject<Physics::VirtualCharacterController>(characterSettings);

	    virtualCharacter->SetPosition(
		    playerNode->GlobalTransform().Position().Value()
	    );

	    virtualCharacter->SetGravityFactor(1.0f);
	    virtualCharacter->SetCollisionLayerAndMask({1}, 0xFFFFFFFF);

	    auto* player =
		    playerNode->AddObject<PlayerController>();

	    player->SetThrowingUnlocked(false);

	    return playerNode;
    }

    inline void CreateCauldronReceiver(Scene& scene, SceneNode* roomNode) {
	    SceneNode* cauldronNode =
		    FindFirstNodeByNameRecursive(roomNode, "Cauldron");

	    if (!cauldronNode) {
		    return;
	    }

	    cauldronNode->AddObject<Crafting::Cauldron>();

	    SceneNode* cauldronReceiverHitboxNode =
		    scene.CreateNode(cauldronNode, "CauldronReceiverHitbox");

	    cauldronReceiverHitboxNode->LocalTransform().Position() =
		    glm::vec3(0.0f, -0.2f, 0.0f);

	    cauldronReceiverHitboxNode->LocalTransform().Scale() =
		    glm::vec3(1.0f);

	    glm::vec3 cauldronReceiverHitboxPosition =
		    cauldronReceiverHitboxNode->GlobalTransform().Position().Value();

	    auto* cauldronBody =
		    cauldronReceiverHitboxNode->AddObject<Physics::Body>(
			    JPH::BodyCreationSettings{
				    Physics::BoxShape(glm::vec3(0.7f, 2.0f, 0.7f)),
				    JPH::RVec3(
					    cauldronReceiverHitboxPosition.x,
					    cauldronReceiverHitboxPosition.y,
					    cauldronReceiverHitboxPosition.z
				    ),
				    JPH::Quat::sIdentity(),
				    JPH::EMotionType::Static,
				    Physics::Layers::NON_MOVING
			    }
		    );

	    cauldronBody->SetIsSensor(true);
	    cauldronBody->SetPosition(cauldronReceiverHitboxPosition);

	    auto* receiver =
		    cauldronReceiverHitboxNode->AddObject<Crafting::CraftingIngredientReceiver>();

	    receiver->receiverHalfExtents =
		    glm::vec3(0.7f, 2.0f, 0.7f);

	    receiver->ingredientConsumeOffset =
		    glm::vec3(0.0f, 0.5f, 0.0f);
    }

    inline void CreateLidHitbox(Scene& scene, SceneNode* roomNode) {
	    SceneNode* lidNode =
		    FindFirstNodeByNameRecursive(roomNode, "Lid");

	    if (!lidNode) {
		    return;
	    }

	    SceneNode* lidHitboxNode =
		    scene.CreateNode(lidNode, "LidHitbox");

	    lidHitboxNode->LocalTransform().Position() =
		    glm::vec3(0.0f, 0.4f, 0.0f);

	    lidHitboxNode->LocalTransform().Scale() =
		    glm::vec3(1.0f);

	    glm::vec3 lidHitboxPosition =
		    lidHitboxNode->GlobalTransform().Position().Value();

	    auto* lidBody = lidHitboxNode->AddObject<Physics::Body>(
		    JPH::BodyCreationSettings{
			    Physics::BoxShape(glm::vec3(0.75f, 0.4f, 0.75f)),
			    JPH::RVec3(
				    lidHitboxPosition.x,
				    lidHitboxPosition.y,
				    lidHitboxPosition.z
			    ),
			    JPH::Quat::sIdentity(),
			    JPH::EMotionType::Kinematic,
			    Physics::Layers::NON_MOVING
		    }
	    );

	    lidBody->SetPosition(lidHitboxPosition);
	    lidBody->SetIsSensor(true);
	    SetInteractionBodyLayer(lidBody);

	    auto* lidInteractable =
		    lidHitboxNode->AddObject<Crafting::CraftingInteractable>();

	    lidInteractable->type = Crafting::CraftingInteractionType::Lid;
	    lidInteractable->interactionEnabled = false;
	    lidInteractable->blinkOutline = true;
    }

    inline void CreateCraftingIngredients(Scene& scene, SceneNode* roomNode) {
	    SceneNode* ingredientsRootNode =
		    scene.CreateNode(roomNode, "Crafting Ingredients");

	    const std::array<std::string, 4> inventorySlots = {
		    "slot_bigger",
		    "slot_bigger.001",
		    "slot_bigger.002",
		    "slot_bigger.003"
	    };

	    std::vector<PotionInventory::IngredientInventoryEntry> ingredientDefinitions =
		    PotionInventory::GetAllIngredientDefinitions();

	    for (int index = 0; index < static_cast<int>(ingredientDefinitions.size()); index++) {
		    const PotionInventory::IngredientInventoryEntry& ingredient =
			    ingredientDefinitions[index];

		    int slotIndex = index % 4;

		    glm::vec3 slotPosition =
			    WorldPointBetweenCameraNodes(
				    roomNode,
				    "StageOneStand",
				    "StageOneLook",
				    0.74f,
				    -0.75f + float(slotIndex) * 0.5f,
				    -0.20f
			    );

		    SceneNode* slotNode =
			    FindFirstNodeByNameRecursive(roomNode, inventorySlots[slotIndex]);

		    if (slotNode) {
			    slotPosition = slotNode->GlobalTransform().Position().Value();
		    }

		    IngredientSpawnData spawn;
		    spawn.nodeName = ingredient.displayName + " Ingredient";
		    spawn.inventoryKey = ingredient.inventoryKey;
		    spawn.modelPath = ingredient.modelPath;
		    spawn.position = slotPosition;
		    spawn.modelScale = glm::vec3(0.42f);
		    spawn.modelRotationEuler = glm::vec3(0.0f, 0.0f, 0.0f);
		    spawn.modelOffset = glm::vec3(0.0f);
		    spawn.interactionHalfExtents = glm::vec3(0.07f, 0.08f, 0.07f);
		    spawn.ingredientData = ingredient.data;

		    Crafting::DraggableCraftingItem* item =
			    CreateDraggableIngredientModel(
				    scene,
				    ingredientsRootNode,
				    spawn
			    );

		    if (item && item->GetNode()) {
			    item->GetNode()->SetEnabled(false);
		    }
	    }

	    ingredientsRootNode->SetEnabled(false);
    }

    inline void SetupCraftingStation(Scene& scene, SceneNode* roomNode) {
	    CreateStationHitbox(scene, roomNode);
	    CreateBlowerHitbox(scene, roomNode);
	    CreateValveHitbox(scene, roomNode);
	    CreateBottlingStageNodes(scene, roomNode);
	    CreateCauldronReceiver(scene, roomNode);
	    CreateLidHitbox(scene, roomNode);

	    auto* craftingStation =
		    roomNode->AddObject<Crafting::CraftingStation>();

	    craftingStation->interactionRadius = 2.6f;

	    bool returnedFromThrowingTutorial =
		    PersistentData::Get<bool>("CraftingScene_ReturnedFromThrowingTutorial");

	    bool shouldAutoEnterCrafting =
		    PersistentData::Get<bool>("CraftingScene_AutoEnterCrafting") &&
		    !returnedFromThrowingTutorial;

	    craftingStation->enterStationOnFirstUpdate =
		    shouldAutoEnterCrafting;

	    PersistentData::Set<bool>("CraftingScene_AutoEnterCrafting", false);
    }

    inline void AddSkybox(Scene& scene, SceneNode* roomNode) {
	    ShaderProgram* skyProg =
		    ShaderProgram::Build()
			    .WithVertexShader("./res/shaders/skybox.vert")
			    .WithPixelShader("./res/shaders/skybox.frag")
			    .Link();

	    Cubemap* skyCubemap =
		    scene.Resources()->Get<Cubemap>(
			    "./res/textures/null_skybox.hdr",
			    Texture::HDRColorBuffer
		    );

	    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
	    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
	    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

	    Material* skyMat =
		    new Material(skyProg);

	    skyMat->SetValue("skyboxTexture", skyCubemap);

	    roomNode->AddObject<Skybox>(skyMat);
    }

    inline void InitScene(Scene& scene) {

	    scene.AddComponent<Physics::System>();
	    scene.AddComponent<LightSystem>();
	    scene.AddComponent<UiSystem>();
	    scene.AddComponent<AnimationSystem>();
	    scene.AddComponent<TweenSystem>();
	    scene.AddComponent<WheelSystem>();

	    if (auto* lightSystem = scene.GetComponent<LightSystem>()) {
		    lightSystem->SetAmbientLight(glm::vec4(1.0f, 0.65f, 0.25f, 0.12f));
	    }

	    SceneNode* sceneRoot =
		    scene.CreateNode("Crafting Scene Root");

	    SceneNode* roomNode =
		    ResourceDatabase::Global->Get<GltfScene>(
			    CraftingRoomModelPath
		    )->Instantiate(&scene, sceneRoot, "Crafting Base Room");

	    if (!roomNode) {
		    return;
	    }

	    AddSkybox(scene, roomNode);
	    AddRoomPhysics(roomNode);
	    CreateFogVolumeAtFogPoint(scene, roomNode);

	    SceneNode* stationNode =
		    CreateCraftingStationModel(scene, roomNode);

	    if (!stationNode) {
		    return;
	    }

	    DisableEmbeddedStationNodes(roomNode, stationNode);

            AddStationMeshPhysics(stationNode);

            CreateCraftingUiButtonInteractables(stationNode);


            SceneNode* stageOneCameraNode =
                    FindFirstNodeByNameRecursive(stationNode, "StageOneCamera");

            if (stageOneCameraNode) {
                SceneNode* stageOneLightNode =
                        scene.CreateNode(stationNode, "StageOneCameraLight");

                stageOneLightNode->GlobalTransform().Position() =
                        stageOneCameraNode->GlobalTransform().Position().Value();

                auto* stageOneLight =
                        stageOneLightNode->AddObject<Light>(
                                Light::PointLight(
                                        glm::vec3(1.0f, 0.62f, 0.28f),
                                        10.0f,
                                        0.0f,
                                        0.12f,
                                        0.045f
                                )
                        );

                stageOneLight->SetIntensity(3.0f);
            }


            SceneNode* stageTwoCameraNode =
                    FindFirstNodeByNameRecursive(stationNode, "StageTwoCamera");

            if (stageTwoCameraNode) {
                SceneNode* stageTwoLightNode =
                        scene.CreateNode(stationNode, "StageTwoCameraLight");

                stageTwoLightNode->GlobalTransform().Position() =
                        stageTwoCameraNode->GlobalTransform().Position().Value();

                auto* stageTwoLight =
                        stageTwoLightNode->AddObject<Light>(
                                Light::PointLight(
                                        glm::vec3(1.0f, 0.62f, 0.28f),
                                        10.0f,
                                        0.0f,
                                        0.12f,
                                        0.045f
                                )
                        );

                stageTwoLight->SetIntensity(3.0f);
            }


            SceneNode* lastStageCameraNode =
                    FindFirstNodeByNameRecursive(stationNode, "LastStageCamera");

            if (lastStageCameraNode) {
                SceneNode* lastStageLightNode =
                        scene.CreateNode(stationNode, "LastStageCameraLight");

                lastStageLightNode->GlobalTransform().Position() =
                        lastStageCameraNode->GlobalTransform().Position().Value();

                auto* lastStageLight =
                        lastStageLightNode->AddObject<Light>(
                                Light::PointLight(
                                        glm::vec3(1.0f, 0.62f, 0.28f),
                                        10.0f,
                                        0.0f,
                                        0.12f,
                                        0.045f
                                )
                        );

                lastStageLight->SetIntensity(3.0f);
            }

	    SceneNode* dragInteractorNode =
		    scene.CreateNode(stationNode, "Crafting Drag Interactor");

	    dragInteractorNode->AddObject<Crafting::CraftingDragInteractor>();

	    SceneNode* playerNode =
		    CreatePlayer(scene, roomNode);

	    SceneNode* cameraNode =
		    scene.CreateNode("Camera Node");

	    cameraNode->AddObject<Camera>(
		    Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f)
	    );

	    cameraNode->GetObject<Camera>()->SetAsMainCamera();

	    cameraNode->AddObject<CameraSettings>(
		    playerNode->GlobalTransform().Position(),
		    7,
		    135
	    );

	    cameraNode->AddObject<MaskEffects>();

	    auto* jfa =
		    cameraNode->AddObject<JfaOutline>();

	    jfa->outlineThickness = 4.0f;
	    jfa->outlineColor = {1.0f, 1.0f, 1.0f};

	    auto* dof =
		    cameraNode->AddObject<DepthOfField>();

	    dof->SetEnabled(false);

	    cameraNode->AddObject<Bloom>();

	    cameraNode->AddObject<Tonemapper>()
		    ->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);

	    cameraNode->AddObject<ColorGrading>();
	    cameraNode->AddObject<Fxaa>();

	    roomNode->AddObject<CraftingRoomLights>();
	    roomNode->AddObject<CraftingTutorialFinishedMessage>();
	    roomNode->AddObject<DungeonEntryPrompt>();

        SceneNode* uiRoot = scene.CreateNode("UI");
        SceneNode* pauseMenu = scene.CreateNode(uiRoot, "Pause Menu");
        pauseMenu->AddObject<PauseMenu>();

        SetupCraftingStation(scene, stationNode);
        CreateCraftingIngredients(scene, stationNode);

    }

}