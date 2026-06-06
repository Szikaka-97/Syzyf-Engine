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

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>
#include <vector>

namespace CraftingScene {

static constexpr const char* CraftingTutorialModelPath =
	"./res/models/rooms/CraftingTutorial.glb";

struct IngredientSpawnData {
	std::string nodeName;
	Material* material = nullptr;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);
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

inline void CollectPointLightsRecursive(SceneNode* node, std::vector<SceneNode*>& pointLights) {
	if (!node) {
		return;
	}

	if (node->GetName().rfind("PointLight", 0) == 0) {
		pointLights.push_back(node);
	}

	for (SceneNode* child : node->GetChildren()) {
		CollectPointLightsRecursive(child, pointLights);
	}
}

inline int GetPointLightIndex(const std::string& name) {
	const std::string prefix = "PointLight.";

	if (name.rfind(prefix, 0) != 0) {
		return 999999;
	}

	try {
		return std::stoi(name.substr(prefix.size()));
	}
	catch (...) {
		return 999999;
	}
}

class CraftingTutorialLights : public GameObject {
private:
	std::vector<Light*> lights;
	std::vector<float> baseIntensities;

	float lightsOnTime = 0.0f;
	float sequenceDelay = 0.45f;
	float fadeDuration = 1.0f;

public:
	void Awake() {
		std::vector<SceneNode*> pointLightNodes;

		CollectPointLightsRecursive(GetNode(), pointLightNodes);

		std::sort(pointLightNodes.begin(), pointLightNodes.end(), [](SceneNode* a, SceneNode* b) {
			return GetPointLightIndex(a->GetName()) < GetPointLightIndex(b->GetName());
		});

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

			light->SetIntensity(0.0f);

			this->lights.push_back(light);
			this->baseIntensities.push_back(3.0f);
		}

		this->lightsOnTime = Time::Current();
	}

	void Update() {
		for (int index = 0; index < this->lights.size(); index++) {
			Light* light = this->lights[index];

			float flicker =
				glm::sin(Time::Current() * (0.6f + (light->GetID() % 4) * 0.15f) + light->GetID() * 4.0f) * 0.35f;

			float targetIntensity =
				this->baseIntensities[index] + flicker;

			float turnOnAmount =
				glm::clamp(
					(Time::Current() - this->lightsOnTime - float(index) * this->sequenceDelay) / this->fadeDuration,
					0.0f,
					1.0f
				);

			light->SetIntensity(targetIntensity * turnOnAmount);
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

inline void CreateCraftingStageCameraPoints(Scene& scene, SceneNode* roomNode) {
	if (!roomNode) {
		return;
	}

	CreateLocalPoint(
		scene,
		roomNode,
		"IngredientCameraPoint",
		glm::vec3(-1.5f, 3.0f, 8.8f)
	);

	CreateLocalPoint(
		scene,
		roomNode,
		"IngredientCameraTarget",
		glm::vec3(-4.2f, 1.5f, 6.6f)
	);

	CreateLocalPoint(
		scene,
		roomNode,
		"HeatingCameraPoint",
		glm::vec3(-2.4f, 2.0f, 5.0f)
	);

	CreateLocalPoint(
		scene,
		roomNode,
		"HeatingCameraTarget",
		glm::vec3(-4.0f, 1.0f, 6.6f)
	);

	CreateLocalPoint(
		scene,
		roomNode,
		"BottlingCameraPoint",
		glm::vec3(-8.2f, 1.6f, 4.4f)
	);

	CreateLocalPoint(
		scene,
		roomNode,
		"BottlingCameraTarget",
		glm::vec3(-8.0f, 0.9f, 6.6f)
	);
}

inline void AddRoomPhysics(SceneNode* roomNode) {
	SceneNode* floorNode = FindFirstNodeByNameRecursive(roomNode, "Plane");
	SceneNode* wallsColliderNode = FindFirstNodeByNameRecursive(roomNode, "Walls Collider");

	if (floorNode) {
		MeshRenderer* floorRenderer = FindMeshRenderer(floorNode);

		if (floorRenderer) {
			auto* floorBody = floorRenderer->GetNode()->AddObject<Physics::Body>(
				JPH::BodyCreationSettings{
					Physics::MeshShape(floorRenderer->GetMesh()),
					JPH::RVec3::sZero(),
					JPH::Quat::sIdentity(),
					JPH::EMotionType::Static,
					Physics::Layers::NON_MOVING
				}
			);

			floorBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
		}

	}

	if (wallsColliderNode) {
		MeshRenderer* wallsRenderer = FindMeshRenderer(wallsColliderNode);

		if (wallsRenderer) {
			auto* wallsBody = wallsRenderer->GetNode()->AddObject<Physics::Body>(
				JPH::BodyCreationSettings{
					Physics::MeshShape(wallsRenderer->GetMesh()),
					JPH::RVec3::sZero(),
					JPH::Quat::sIdentity(),
					JPH::EMotionType::Static,
					Physics::Layers::NON_MOVING
				}
			);

			wallsBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
		}
	}
}

inline SceneNode* CreateStationHitbox(Scene& scene, SceneNode* roomNode) {
	if (!roomNode) {
		return nullptr;
	}

	SceneNode* stationHitboxNode =
		scene.CreateNode(roomNode, "StationHitbox");

	stationHitboxNode->LocalTransform().Position() =
		glm::vec3(-5.2f, 1.2f, 6.6f);

	stationHitboxNode->LocalTransform().Scale() =
		glm::vec3(1.0f);

	glm::vec3 stationHitboxPosition =
		stationHitboxNode->GlobalTransform().Position().Value();

	auto* stationBody = stationHitboxNode->AddObject<Physics::Body>(
		JPH::BodyCreationSettings{
			Physics::BoxShape(glm::vec3(2.4f, 1.5f, 2.4f)),
			JPH::RVec3(
				stationHitboxPosition.x,
				stationHitboxPosition.y,
				stationHitboxPosition.z
			),
			JPH::Quat::sIdentity(),
			JPH::EMotionType::Static,
			Physics::Layers::NON_MOVING
		}
	);

	stationBody->SetPosition(stationHitboxPosition);

	return stationHitboxNode;
}

inline SceneNode* CreateBlowerHitbox(Scene& scene, SceneNode* roomNode) {
	if (!roomNode) {
		return nullptr;
	}

	SceneNode* firePlaceNode =
		FindFirstNodeByNameRecursive(roomNode, "Fire_Place");

	if (!firePlaceNode) {

		firePlaceNode = roomNode;
	}

	SceneNode* blowerHitboxNode =
		scene.CreateNode(firePlaceNode, "BlowerHitbox");

	blowerHitboxNode->LocalTransform().Position() =
		glm::vec3(0.0f, 0.15f, 0.0f);

	blowerHitboxNode->LocalTransform().Scale() =
		glm::vec3(0.45f);

	Mesh* cubeMesh =
		scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

	Material* blowerMaterial =
		CreateColorMaterial(glm::vec4(0.85f, 0.2f, 1.0f, 0.45f));

	if (cubeMesh && blowerMaterial) {
		blowerHitboxNode->AddObject<MeshRenderer>(
			cubeMesh,
			blowerMaterial
		);
	}

	glm::vec3 blowerHitboxPosition =
		blowerHitboxNode->GlobalTransform().Position().Value();

	auto* blowerBody = blowerHitboxNode->AddObject<Physics::Body>(
		JPH::BodyCreationSettings{
			Physics::BoxShape(glm::vec3(0.25f)),
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

	auto* blowerInteractable =
		blowerHitboxNode->AddObject<Crafting::CraftingInteractable>();

	blowerInteractable->type = Crafting::CraftingInteractionType::Blower;
	blowerInteractable->interactionEnabled = false;

	return blowerHitboxNode;
}

inline SceneNode* CreateDoorHitbox(Scene& scene, SceneNode* roomNode) {
	if (!roomNode) {
		return nullptr;
	}

	SceneNode* doorNode =
		FindFirstNodeByNameRecursive(roomNode, "Door");

	if (!doorNode) {
		return nullptr;
	}

	SceneNode* doorHitboxNode =
		scene.CreateNode(doorNode, "DoorHitbox");

	doorHitboxNode->LocalTransform().Position() =
		glm::vec3(0.0f);

	doorHitboxNode->LocalTransform().Scale() =
		glm::vec3(0.7f, 0.8f, 0.35f);

	Mesh* cubeMesh =
		scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

	Material* doorMaterial =
		CreateColorMaterial(glm::vec4(0.2f, 0.7f, 1.0f, 0.45f));

	if (cubeMesh && doorMaterial) {
		doorHitboxNode->AddObject<MeshRenderer>(
			cubeMesh,
			doorMaterial
		);
	}

	glm::vec3 doorHitboxPosition =
		doorHitboxNode->GlobalTransform().Position().Value();

	auto* doorBody = doorHitboxNode->AddObject<Physics::Body>(
		JPH::BodyCreationSettings{
			Physics::BoxShape(glm::vec3(0.6f, 0.7f, 0.25f)),
			JPH::RVec3(
				doorHitboxPosition.x,
				doorHitboxPosition.y,
				doorHitboxPosition.z
			),
			JPH::Quat::sIdentity(),
			JPH::EMotionType::Static,
			Physics::Layers::NON_MOVING
		}
	);

	doorBody->SetPosition(doorHitboxPosition);

	auto* doorInteractable =
		doorHitboxNode->AddObject<Crafting::CraftingInteractable>();

	doorInteractable->type = Crafting::CraftingInteractionType::Door;
	doorInteractable->interactionEnabled = false;

	return doorHitboxNode;
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
		valveNode = roomNode;
	}

	SceneNode* valveHitboxNode =
		scene.CreateNode(valveNode, "ValveHitbox");

	valveHitboxNode->LocalTransform().Position() =
		glm::vec3(0.0f);

	valveHitboxNode->LocalTransform().Scale() =
		glm::vec3(0.35f);

	Mesh* cubeMesh =
		scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

	Material* valveMaterial =
		CreateColorMaterial(glm::vec4(1.0f, 0.85f, 0.15f, 0.45f));

	if (cubeMesh && valveMaterial) {
		valveHitboxNode->AddObject<MeshRenderer>(
			cubeMesh,
			valveMaterial
		);
	}

	glm::vec3 valveHitboxPosition =
		valveHitboxNode->GlobalTransform().Position().Value();

	auto* valveBody = valveHitboxNode->AddObject<Physics::Body>(
		JPH::BodyCreationSettings{
			Physics::BoxShape(glm::vec3(0.3f)),
			JPH::RVec3(
				valveHitboxPosition.x,
				valveHitboxPosition.y,
				valveHitboxPosition.z
			),
			JPH::Quat::sIdentity(),
			JPH::EMotionType::Static,
			Physics::Layers::NON_MOVING
		}
	);

	valveBody->SetPosition(valveHitboxPosition);

	auto* valveInteractable =
		valveHitboxNode->AddObject<Crafting::CraftingInteractable>();

	valveInteractable->type = Crafting::CraftingInteractionType::Valve;
	valveInteractable->interactionEnabled = false;

	return valveHitboxNode;
}

inline Material* CreateColorMaterial(const glm::vec4& color) {
	ShaderProgram* shader =
		ShaderProgram::Build()
			.WithVertexShader("./res/shaders/lit.vert")
			.WithPixelShader("./res/shaders/transparent.frag")
			.Link();

	Material* material = new Material(shader);
	material->SetValue("uColor", color);

	return material;
}

inline SceneNode* CreateBottlingDebugCube(
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

	const glm::vec3 bottleStartPoint =
		glm::vec3(-6.8f, 0.55f, 7.5f);

	const glm::vec3 bottleFillPoint =
		glm::vec3(-8.0f, 0.55f, 6.65f);

	const glm::vec3 bottleEndPoint =
		glm::vec3(-9.2f, 0.55f, 5.8f);

	const glm::vec3 lanePosition =
		glm::vec3(-8.0f, 0.35f, 6.65f);

	const glm::vec3 laneScale =
		glm::vec3(2.8f, 0.08f, 0.45f);

	const glm::vec3 fillZoneScale =
		glm::vec3(0.34f, 0.55f, 0.34f);

	const glm::vec3 valveGuidePosition =
		glm::vec3(-8.0f, 1.0f, 6.65f);

	const glm::vec3 valveGuideScale =
		glm::vec3(0.08f, 0.42f, 0.08f);

	const glm::vec3 bottleScale =
		glm::vec3(0.18f, 0.45f, 0.18f);

	CreateLocalPoint(
		scene,
		roomNode,
		"BottleStartPoint",
		bottleStartPoint
	);

	CreateLocalPoint(
		scene,
		roomNode,
		"BottleFillPoint",
		bottleFillPoint
	);

	CreateLocalPoint(
		scene,
		roomNode,
		"BottleEndPoint",
		bottleEndPoint
	);

	Mesh* cubeMesh =
		scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

	Material* bottleMaterial =
		CreateColorMaterial(glm::vec4(0.35f, 0.75f, 1.0f, 0.9f));

	Material* fillZoneMaterial =
		CreateColorMaterial(glm::vec4(0.1f, 1.0f, 0.25f, 0.45f));

	Material* beltMaterial =
		CreateColorMaterial(glm::vec4(0.6f, 0.6f, 0.6f, 0.35f));

	Material* liquidMaterial =
		CreateColorMaterial(glm::vec4(0.9f, 0.25f, 0.15f, 0.95f));

	Material* valveGuideMaterial =
		CreateColorMaterial(glm::vec4(1.0f, 0.85f, 0.15f, 0.45f));

	SceneNode* bottlesRoot =
		scene.CreateNode(roomNode, "BottlingBottlesRoot");

	bottlesRoot->LocalTransform().Position() =
		glm::vec3(0.0f);

	CreateBottlingDebugCube(
		scene,
		bottlesRoot,
		"BottlingLaneDebug",
		cubeMesh,
		beltMaterial,
		lanePosition,
		laneScale
	);

	CreateBottlingDebugCube(
		scene,
		bottlesRoot,
		"BottleFillZoneDebug",
		cubeMesh,
		fillZoneMaterial,
		bottleFillPoint,
		fillZoneScale
	);

	CreateBottlingDebugCube(
		scene,
		bottlesRoot,
		"ValveToBottleGuideDebug",
		cubeMesh,
		valveGuideMaterial,
		valveGuidePosition,
		valveGuideScale
	);

	for (int i = 0; i < 4; ++i) {
		SceneNode* bottleNode = CreateBottlingDebugCube(
			scene,
			bottlesRoot,
			"BottlingBottle_0" + std::to_string(i + 1),
			cubeMesh,
			bottleMaterial,
			bottleStartPoint,
			bottleScale
		);

		SceneNode* liquidNode = CreateBottlingDebugCube(
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

inline Crafting::DraggableCraftingItem* CreateDraggableCube(
	Scene& scene,
	SceneNode* parent,
	const std::string& nodeName,
	Mesh* mesh,
	Material* material,
	const glm::vec3& position,
	const glm::vec3& scale,
	const Crafting::IngredientData& ingredientData
) {
	SceneNode* node = scene.CreateNode(parent, nodeName);

	node->AddObject<MeshRenderer>(mesh, material);

	node->LocalTransform().Position() = position;
	node->LocalTransform().Scale() = scale;

	glm::vec3 globalPosition =
		node->GlobalTransform().Position().Value();

	auto* item = node->AddObject<Crafting::DraggableCraftingItem>();
	item->data = ingredientData;

	auto* interactable = node->AddObject<Crafting::CraftingInteractable>();
	interactable->type = Crafting::CraftingInteractionType::Ingredient;
	interactable->interactionEnabled = true;

	auto* body = node->AddObject<Physics::Body>(
		JPH::BodyCreationSettings{
			Physics::BoxShape(scale),
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

	return item;
}

inline SceneNode* CreatePlayer(Scene& scene, SceneNode* roomNode) {
	JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
		new JPH::CharacterVirtualSettings();

	characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
	characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
	characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

	SceneNode* playerNode = scene.CreateNode("Player");

	SceneNode* spawnNode =
		FindFirstNodeByNamesRecursive(
			roomNode,
			{
				"spawn",
				"Spawn",
				"PlayerSpawn",
				"Exit Gate.001",
				"Entrance"
			}
		);

	if (spawnNode) {
		playerNode->GlobalTransform().Position() =
			spawnNode->GlobalTransform().Position().Value();
	}
	else {
		playerNode->GlobalTransform().Position() =
			glm::vec3(1.7f, 0.0f, -10.4f);
	}

	ResourceDatabase::Global->Get<GltfScene>(
		"./res/models/bimbermann_throwing.glb"
	)->Instantiate(&scene, playerNode, "Bimberman");

	SceneNode* aimReticle =
		ResourceDatabase::Global->Get<GltfScene>(
			"./res/models/crosshair.glb"
		)->Instantiate(&scene, playerNode, "Aim Reticle");

	aimReticle->AddObject<AimCrosshair>();
	aimReticle->SetEnabled(false);

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
		spdlog::error("CraftingScene: Cauldron not found.");
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
		spdlog::error("CraftingScene: Lid not found.");
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

	auto* lidInteractable =
		lidHitboxNode->AddObject<Crafting::CraftingInteractable>();

	lidInteractable->type = Crafting::CraftingInteractionType::Lid;
	lidInteractable->interactionEnabled = false;
}

inline void CreateCraftingIngredients(Scene& scene, SceneNode* roomNode) {
	Mesh* cubeMesh =
		scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

	const glm::vec4 burnColor =
		glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);

	const glm::vec4 lightningColor =
		glm::vec4(1.0f, 1.0f, 0.1f, 1.0f);

	const glm::vec4 radiusColor =
		glm::vec4(0.1f, 0.8f, 0.2f, 1.0f);

	const glm::vec4 durationColor =
		glm::vec4(0.1f, 0.3f, 1.0f, 1.0f);

	Material* burnMaterial =
		CreateColorMaterial(burnColor);

	Material* lightningMaterial =
		CreateColorMaterial(lightningColor);

	Material* radiusMaterial =
		CreateColorMaterial(radiusColor);

	Material* durationMaterial =
		CreateColorMaterial(durationColor);

	SceneNode* ingredientsRootNode =
		scene.CreateNode(roomNode, "Crafting Ingredients");

	const std::vector<IngredientSpawnData> ingredientSpawns = {
		{
			"Burn Ingredient",
			burnMaterial,
			glm::vec3(-2.8f, 1.1f, 7.8f),
			glm::vec3(0.35f),
			CreateMainEffectIngredient(
				Crafting::IngredientType::Sugar,
				"Burn",
				Crafting::EffectId::Burn,
				burnColor
			)
		},
		{
			"Lightning Ingredient",
			lightningMaterial,
			glm::vec3(-2.8f, 1.1f, 7.1f),
			glm::vec3(0.35f),
			CreateMainEffectIngredient(
				Crafting::IngredientType::Water,
				"Lightning",
				Crafting::EffectId::Lightning,
				lightningColor
			)
		},
		{
			"Radius Modifier",
			radiusMaterial,
			glm::vec3(-2.8f, 1.1f, 6.4f),
			glm::vec3(0.35f),
			CreateModifierIngredient(
				Crafting::IngredientType::Water,
				"Radius",
				Crafting::ModifierId::Radius,
				1.0f,
				radiusColor
			)
		},
		{
			"Duration Modifier",
			durationMaterial,
			glm::vec3(-2.8f, 1.1f, 5.7f),
			glm::vec3(0.35f),
			CreateModifierIngredient(
				Crafting::IngredientType::Sugar,
				"Duration",
				Crafting::ModifierId::Duration,
				1.0f,
				durationColor
			)
		}
	};

	for (const IngredientSpawnData& spawn : ingredientSpawns) {
		CreateDraggableCube(
			scene,
			ingredientsRootNode,
			spawn.nodeName,
			cubeMesh,
			spawn.material,
			spawn.position,
			spawn.scale,
			spawn.ingredientData
		);
	}

	ingredientsRootNode->SetEnabled(false);
}

inline void SetupCraftingStation(Scene& scene, SceneNode* roomNode) {
	CreateCraftingStageCameraPoints(scene, roomNode);
	CreateStationHitbox(scene, roomNode);
	CreateBlowerHitbox(scene, roomNode);
	CreateDoorHitbox(scene, roomNode);
	CreateValveHitbox(scene, roomNode);
	CreateBottlingStageNodes(scene, roomNode);
	CreateCauldronReceiver(scene, roomNode);
	CreateLidHitbox(scene, roomNode);

	auto* craftingStation =
		roomNode->AddObject<Crafting::CraftingStation>();

	craftingStation->interactionRadius = 3.0f;

	craftingStation->stationCameraPosition =
		glm::vec3(-4.2f, 5.0f, 6.6f);

	craftingStation->stationCameraRotation =
		glm::quat(glm::radians(glm::vec3(60.0f, -90.0f, 0.0f)));
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

	if (auto* lightSystem = scene.GetComponent<LightSystem>()) {
		lightSystem->SetAmbientLight(glm::vec4(1.0f, 0.65f, 0.25f, 0.12f));
	}

	SceneNode* sceneRoot =
		scene.CreateNode("Crafting Scene Root");

	SceneNode* roomNode =
		ResourceDatabase::Global->Get<GltfScene>(
			CraftingTutorialModelPath
		)->Instantiate(&scene, sceneRoot, "Crafting Tutorial Room");

	if (!roomNode) {
		spdlog::error("CraftingScene: failed to load CraftingTutorial.glb.");
		return;
	}

	AddSkybox(scene, roomNode);
	AddRoomPhysics(roomNode);

	SceneNode* dragInteractorNode =
		scene.CreateNode(roomNode, "Crafting Drag Interactor");

	dragInteractorNode->AddObject<Crafting::CraftingDragInteractor>();

	SceneNode* playerNode =
		CreatePlayer(scene, roomNode);

	SceneNode* cameraNode =
		scene.CreateNode("Camera Node");

	cameraNode->AddObject<Camera>(
		Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f)
	);

	cameraNode->GetObject<Camera>()->SetAsMainCamera();

	auto* cameraSettings =
		cameraNode->AddObject<CameraSettings>(
			playerNode->GlobalTransform().Position()
		);

	cameraSettings->height = 5.0f;
	cameraSettings->angleY = 135.0f;
	cameraSettings->angleX = 45.0f;

	cameraNode->AddObject<MaskEffects>();

	auto* jfa =
		cameraNode->AddObject<JfaOutline>();

	jfa->outlineThickness = 4.0f;
	jfa->outlineColor = {1.0f, 29.0f / 255.0f, 29.0f / 255.0f};

	auto* dof =
		cameraNode->AddObject<DepthOfField>();

	dof->SetEnabled(false);

	cameraNode->AddObject<Bloom>();

	cameraNode->AddObject<Tonemapper>()
		->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);

	cameraNode->AddObject<ColorGrading>();
	cameraNode->AddObject<Fxaa>();

	roomNode->AddObject<CraftingTutorialLights>();

	SetupCraftingStation(scene, roomNode);
	CreateCraftingIngredients(scene, roomNode);

}

}