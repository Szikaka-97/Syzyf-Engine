#pragma once

#include "DepthOfField.h"
#include "EasingFunctions.h"
#include "GltfImporter.h"
#include "LightSystem.h"
#include "game_scripts/AimingAid.h"
#include "game_scripts/ThrowBottle.h"
#include <GameScripts/DungeonGenerator.h>

#include <AiNode.h>
#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
#include <Framebuffer.h>
#include <Fxaa.h>
#include <InputSystem.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <MeshRenderer.h>
#include <Mirror.h>
#include <ParticleSpawner.h>
#include <ReflectionProbe.h>
#include <Scene.h>
#include <Shader.h>
#include <Skybox.h>
#include <TimeSystem.h>
#include <Tonemapper.h>
#include <TweenSystem.h>
#include <Viewport.h>
#include <animation/AnimationSystem.h>
#include <fog/FogVolume.h>
#include <game_scripts/CameraSettings.h>
#include <game_scripts/PlayerController.h>
#include <game_scripts/ThrowBottle.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <physics/Body.h>
#include <physics/DebugRenderer.h>
#include <physics/Helpers.h>
#include <physics/System.h>
#include <physics/Water.h>
#include <scatter/Spawner.h>

#include "Jolt/Math/Vec3.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <imgui.h>
#include <physics/VirtualCharacterController.h>

namespace DungeonGeneratorScene {
class EditorCameraTag : public GameObject {};

inline void InitScene(Scene& mainScene) {
	mainScene.AddComponent<Physics::System>();
	mainScene.AddComponent<DebugInspector>();
	mainScene.AddComponent<AnimationSystem>();
	auto* tweenSystem = mainScene.AddComponent<TweenSystem>();

// If Visual Studio doesn't like this I'm going to give up and force you guys to
// switch to GCC
#pragma region World

	ShaderProgram* skyProg = ShaderProgram::Build()
	.WithVertexShader(("./res/shaders/skybox.vert"))
	.WithPixelShader(("./res/shaders/skybox.frag"))
	.Link();

	Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
		"./res/textures/skybox_showcase.hdr",
		Texture::HDRColorBuffer
	);
	skyCubemap->SetWrapModeU(TextureWrap::Clamp);
	skyCubemap->SetWrapModeV(TextureWrap::Clamp);
	skyCubemap->SetWrapModeW(TextureWrap::Clamp);

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	mainScene.GetRootNode()->AddObject<Skybox>(skyMat);

#pragma endregion
#pragma region Generator

	SceneNode* generatorNode = mainScene.CreateNode("Dungeon Generator");
	generatorNode->AddObject<DungeonGenerator>()->RemakeDungeon();

#pragma endregion
#pragma region Player

	JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
		new JPH::CharacterVirtualSettings();
	characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
	characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
	characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

	SceneNode* playerNode = mainScene.CreateNode("Player");

	SceneNode* bimberman = GltfImporter::LoadScene(&mainScene, "./res/models/bimbermann_throwing.glb", "Bimberman");
	bimberman->SetParent(playerNode);

#pragma endregion
#pragma region Camera

	SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
	cameraNode->AddObject<Camera>(
		Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
	auto* cameraController = cameraNode->AddObject<CameraSettings>(playerNode->GlobalTransform().Position());
	cameraController->angleY = 135;
	cameraController->height = 10;
	cameraNode->AddObject<Bloom>();
	cameraNode->AddObject<Tonemapper>()->SetOperator(
		Tonemapper::TonemapperOperator::GranTurismo);
	cameraNode->AddObject<ColorGrading>();
	cameraNode->AddObject<Fxaa>();

#pragma endregion
#pragma region Miscellaneous
	SceneNode* sun = mainScene.CreateNode("Sun");
	sun->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 2))
		->SetShadowCasting(true);
	sun->GlobalTransform().Position() = {1, 2.2f, 0};
	sun->GlobalTransform().Rotation() =
		glm::quat(glm::radians(glm::vec3(50.0f, -20.0f, 0.0f)));
	mainScene.GetComponent<LightSystem>()->SetAmbientLight(
		{1.0f, 1.0f, 1.0f, 0.6f});

	// Mesh* mirrorMesh =
	//     mainScene.Resources()->Get<Mesh>("./res/models/plane.obj");
	// SceneNode* mirrorNode = mainScene.CreateNode("Mirror");
	// SceneNode* mirrorMeshNode = mainScene.CreateNode(mirrorNode, "Mirror
	// Mesh"); mirrorMeshNode->AddObject<Mirror>(mirrorMesh);
	// mirrorNode->GlobalTransform().Position() = {15.0f, 0.0f, 1.5f};
	// mirrorNode->GlobalTransform().Rotation() =
	//     glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 0.0f)));
	// mirrorNode->GetObjectInChildren<MeshRenderer>()->GlobalTransform().Scale()
	// =
	//     {10.0f, 7.0f, 1.0f};
	// Physics::CreateCompoundShapeFromNode(
	//     mirrorNode->GetObjectInChildren<MeshRenderer>()->GetNode(), false,
	//     JPH::EMotionType::Static, Physics::Layers::NON_MOVING);

	// SceneNode* fogVolume = mainScene.CreateNode("Fog Volume");
	// fogVolume->AddObject<FogVolume>();
	// fogVolume->GlobalTransform().Position() = {28.0f, 0.25f, 0.0f};
	// fogVolume->GlobalTransform().Scale() = {200.0f, 0.5f, 100.0f};


#pragma endregion

	return;
}
} // namespace DungeonGeneratorScene
