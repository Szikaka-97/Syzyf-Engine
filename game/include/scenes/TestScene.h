#pragma once

#include "DepthOfField.h"
#include "FastNoiseLite.h"
#include "GameObjectSystem.h"
#include "GltfScene.h"
#include "JfaOutline.h"
#include "LightSystem.h"
#include "Noise3D.h"
#include "Resources.h"
#include "audio/AudioClip.h"
#include "audio/AudioSource.h"
#include "audio/AudioSystem.h"
#include "fog/FluidSimulation.h"
#include "fog/Fog.h"
#include "game_scripts/FireParticles.h"
#include "game_scripts/PickableItemSystem.h"
#include "game_scripts/ui/InGame.h"
#include "game_scripts/ui/PauseMenu.h"
#include "game_scripts/ui/TabMenu.h"
#include "ui/widgets/wheel/UiWheel.h"
#include <game_scripts/AimCrosshair.h>
#include <game_scripts/PlayerController.h>

#include "game_scripts/PotionInventory.h"
#include "game_scripts/enemies/FlockingSystem.h"
#include "game_scripts/enemies/MeleeSkeleton.h"
#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
#include <DepthOfField.h>
#include <Formatters.h>
#include <Framebuffer.h>
#include <Fxaa.h>
#include <InputSystem.h>
#include <Light.h>
#include <MaskEffects.h>
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
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/enemies/EnemySkeleton.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/trigonometric.hpp>
#include <physics/Body.h>
#include <physics/DebugRenderer.h>
#include <physics/Helpers.h>
#include <physics/System.h>
#include <physics/Water.h>
#include <scatter/Spawner.h>
#include <text/Font.h>
#include <ui/objects/UiCursor.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiScrollableGrid.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/UiCircularBar.h>
#include <ui/widgets/wheel/UiRadialWheel.h>

#include "Jolt/Math/Vec3.h"
#include "text/Text3D.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <imgui.h>
#include <physics/VirtualCharacterController.h>

#include <Profiler.h>

namespace TestScene {

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<AnimationSystem>();
    mainScene.AddComponent<PickableItemSystem>();
    auto* tweenSystem = mainScene.AddComponent<TweenSystem>();
    mainScene.AddComponent<WheelSystem>();
    mainScene.AddComponent<ThrowableObjectPool>();
    mainScene.AddComponent<AudioSystem>();

    mainScene.GetGraphics()->ssaoSettings.enabled = false;

// If Visual Studio doesn't like this I'm going to give up and force you guys to
// switch to GCC
#pragma region World

    ShaderProgram* skyProg =
        ShaderProgram::Build()
            .WithVertexShader(("./res/shaders/skybox.vert"))
            .WithPixelShader(("./res/shaders/skybox.frag"))
            .Link();

    Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
        "./res/textures/citrus_orchard_road_puresky.hdr",
        Texture::HDRColorBuffer);
    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

    Material* skyMat = new Material(skyProg);
    skyMat->SetValue("skyboxTexture", skyCubemap);

    auto floorNode =
        ResourceDatabase::Global->Get<GltfScene>("./res/models/floor.glb")
            ->Instantiate(&mainScene, mainScene.root, "Floor");
    floorNode->AddObject<Skybox>(skyMat);
    MeshRenderer* floorMeshRenderer =
        floorNode->GetObjectInChildren<MeshRenderer>();
    auto* floorBody = floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
        JPH::BodyCreationSettings{
            Physics::MeshShape(floorMeshRenderer->GetMesh()),
            JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
            Physics::Layers::NON_MOVING});
    auto* surface =
        floorNode->AddObject<Surface>(floorMeshRenderer->GetMesh(), 1.0f);
    floorBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

    // auto room1 =
    //     ResourceDatabase::Global->Get<GltfScene>("./res/models/floor/room1.glb")
    //         ->Instantiate(&mainScene, mainScene.root, "Room 1");
    // MeshRenderer* room1MeshRenderer =
    //     room1->GetObjectInChildren<MeshRenderer>();
    // auto* room1Body = room1MeshRenderer->GetNode()->AddObject<Physics::Body>(
    //     JPH::BodyCreationSettings{
    //         Physics::MeshShape(room1MeshRenderer->GetMesh()),
    //         JPH::RVec3::sZero(), JPH::Quat::sZero(),
    //         JPH::EMotionType::Static, Physics::Layers::NON_MOVING});
    // auto* room1Surface =
    //     room1->AddObject<Surface>(room1MeshRenderer->GetMesh(), 1.0f);

    // room1Surface->GetObject<Surface>()->SetID(1);
    // room1Body->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

    // auto room2 =
    //     ResourceDatabase::Global->Get<GltfScene>("./res/models/floor/room2.glb")
    //         ->Instantiate(&mainScene, mainScene.root, "Room 2");
    // MeshRenderer* room2MeshRenderer =
    //     room2->GetObjectInChildren<MeshRenderer>();
    // auto* room2Body = room2MeshRenderer->GetNode()->AddObject<Physics::Body>(
    //     JPH::BodyCreationSettings{
    //         Physics::MeshShape(room2MeshRenderer->GetMesh()),
    //         JPH::RVec3::sZero(), JPH::Quat::sZero(),
    //         JPH::EMotionType::Static, Physics::Layers::NON_MOVING});
    // auto* room2Surface =
    //     room2->AddObject<Surface>(room2MeshRenderer->GetMesh(), 1.0f);

    // room2Surface->GetObject<Surface>()->SetID(1);
    // room2Body->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

#pragma endregion
#pragma region Player

    JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
        new JPH::CharacterVirtualSettings();
    characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
    characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

    SceneNode* playerNode = mainScene.CreateNode("Player");
    playerNode->GlobalTransform().Position() = glm::vec3(3, 0, 0);

    SceneNode* bimberman =
        ResourceDatabase::Global
            ->Get<GltfScene>("./res/models/bimbermann_throwing.glb")
            ->Instantiate(&mainScene, mainScene.root, "Bimberman");
    bimberman->SetParent(playerNode);

    auto* virtualCharacter =
        playerNode->AddObject<Physics::VirtualCharacterController>(
            characterSettings);
    virtualCharacter->SetCollisionLayerAndMask({1}, 0xFFFFFFFF);
    virtualCharacter->SetPosition(
        playerNode->GlobalTransform().Position().Value());
    virtualCharacter->SetGravityFactor(1);

    auto* aimingAid =
        ResourceDatabase::Global->Get<GltfScene>("./res/models/crosshair.glb")
            ->Instantiate(&mainScene, playerNode, "Aim Reticle")
            ->AddObject<AimCrosshair>();

    auto* player = playerNode->AddObject<PlayerController>();

    PotionInventory::SaveLastCraftedPotion("Basic Potion", "Explosion", 100.0f,
                                           999, false);

#pragma endregion

#pragma region Camera

    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(
        playerNode->GlobalTransform().Position());
    cameraNode->AddObject<MaskEffects>();
    auto* jfa = cameraNode->AddObject<JfaOutline>();
    jfa->outlineThickness = 4.0f;
    jfa->outlineColor = {1.0f, 29.0f / 255.0f, 29.0f / 255.0f};
    auto* dof = cameraNode->AddObject<DepthOfField>();
    dof->SetEnabled(false);
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

#pragma endregion
#pragma region Enemy

    // auto* flockingSystem = mainScene.AddComponent<FlockingSystem>();
    // flockingSystem->separationRadius = 2.5f;
    // flockingSystem->separationWeight = 1.8f;
    // flockingSystem->alignmentRadius = 5.0f;
    // flockingSystem->alignmentWeight = 0.3f;
    // flockingSystem->cohesionRadius = 6.0f;
    // flockingSystem->cohesionWeight = 0.2f;
    // JPH::ShapeRefC enemyShape = new JPH::CapsuleShape(0.5f, 1.0f);
    // JPH::BodyCreationSettings enemySettingsTemplate(
    //     enemyShape, JPH::RVec3(0, 1.5f, 0), JPH::Quat::sIdentity(),
    //     JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
    // Material* enemyMat =
    //     mainScene.Resources()->Get<Material>("./res/materials/jake.mat");
    // Mesh* cubeMesh =
    //     mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");
    //
    // SceneNode* enemy1 = mainScene.CreateNode("Enemy 1");
    // // enemy1->GlobalTransform().Position() = glm::vec3(10.5f, 0.0f, -5.0f);
    // enemy1->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
    // enemy1->GlobalTransform().Position() = glm::vec3(15.f, 0.f, 0.f);
    // Physics::Body* enemyBody1 =
    // enemy1->AddObject<Physics::Body>(enemySettings);
    // enemyBody1->SetRestitution(0.0f);
    // auto* enemyAi1 = enemy1->AddObject<EnemySkeleton>();
    // enemyAi1->SetSurface(surface);
    // enemyAi1->SetTargetNode(player->GetNode());
    // enemyAi1->SetProjectileResources(cubeMesh, enemyMat);
    // enemyAi1->SetAttackCooldown(1.2f);
    // enemyAi1->SetRoomID(floorNode->GetID());

    // SceneNode* enemyModel =
    //     ResourceDatabase::Global->Get<GltfScene>("./res/models/szkielet6.glb")
    //         ->Instantiate(&mainScene, mainScene.root, "EnemyModel");
    // enemyModel->SetParent(enemy1);
    // enemyModel->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);
    // enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();

#pragma endregion

    // Torch fire particles
    SceneNode* sprayNode = mainScene.CreateNode("Fire");
    sprayNode->GlobalTransform().Position() = {0.0f, 0.0f, 0.0f};
    sprayNode->AddObject<FireParticles>();

    // Szkielet blendowanie
    SceneNode* skeletonNode =
        ResourceDatabase::Global
            ->Get<GltfScene>("./res/models/enemies/szkielet4.glb")
            ->Instantiate(&mainScene, mainScene.root, "SkeletonNode");

    auto* animComp = skeletonNode->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        animComp->SetAnimationLayer("walk.001", 0);
        animComp->SetAnimationLayer("attack.001", 1);

        SceneNode* spineNode =
            skeletonNode->FindNode("rig_deform/DEF-upper_arm.L");
        if (spineNode) {
            animComp->SetAnimationMask("attack.001", spineNode);
        }

        animComp->Play("walk.001");
        animComp->Play("attack.001");

        for (auto& anim : animComp->animations) {
            if (anim.data.name == "walk.001" ||
                anim.data.name == "attack.001") {
                anim.looping = true;
            }
            if (anim.data.name == "attack.001") {
                anim.blendWeight = 1.0f;
            }
        }
    }

    SceneNode* audio = mainScene.CreateNode("Audio");
    audio->AddObject<AudioSource>(
        mainScene.Resources()->Get<AudioClip>("./res/audio/Click.wav"));

    // UI
    SceneNode* uiRoot = mainScene.CreateNode("UI");
    SceneNode* tabMenu = mainScene.CreateNode(uiRoot, "Tab Menu");
    tabMenu->AddObject<TabMenu>();
    SceneNode* pauseMenu = mainScene.CreateNode(uiRoot, "Pause Menu");
    pauseMenu->AddObject<PauseMenu>();
    SceneNode* inGameUi = mainScene.CreateNode("HUD");
    inGameUi->AddObject<InGameUi>();
}
} // namespace TestScene
