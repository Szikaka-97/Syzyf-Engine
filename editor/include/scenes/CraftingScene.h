#pragma once

#include "Camera.h"
#include "GltfImporter.h"
#include "Light.h"
#include "LightSystem.h"
#include "Material.h"
#include "Mesh.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "Shader.h"
#include "Tonemapper.h"
#include <TimeSystem.h>
#include <TweenSystem.h>

#include "game_scripts/AimingAid.h"
#include "game_scripts/CameraSettings.h"
#include "game_scripts/crafting/CraftingDragInteractor.h"
#include "game_scripts/crafting/DraggableCraftingItem.h"
#include "game_scripts/crafting/CraftingStation.h"

#include <Player.h>
#include <game_scripts/PlayerController.h>
#include <physics/VirtualCharacterController.h>

#include <animation/AnimationSystem.h>
#include <ui/systems/UiSystem.h>

#include <physics/Body.h>
#include <physics/Helpers.h>
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

#include <string>

namespace CraftingScene{
    inline void AddStaticPhysicsFromModel(SceneNode* modelNode){
        if (!modelNode){
            spdlog::warn("CraftingScene: cannot add physics to null model node.");
            return;
        }

        JPH::ShapeRefC shape = Physics::CreateCompoundShapeFromNode(
            modelNode,
            false,
            JPH::EMotionType::Static,
            Physics::Layers::NON_MOVING
        );

        modelNode->AddObject<Physics::Body>(
            JPH::BodyCreationSettings{
                shape,
                JPH::Vec3::sZero(),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Static,
                Physics::Layers::NON_MOVING
            }
        );
    }

    inline Material* CreateColorMaterial(const glm::vec4& color){
        ShaderProgram* shader =
            ShaderProgram::Build()
                .WithVertexShader("./res/shaders/lit.vert")
                .WithPixelShader("./res/shaders/transparent.frag")
                .Link();

        Material* material = new Material(shader);
        material->SetValue("uColor", color);

        return material;
    }

    inline Crafting::DraggableCraftingItem* CreateDraggableCube(
        Scene& scene, SceneNode* parent, const std::string& nodeName, Mesh* mesh,
        Material* material, const glm::vec3& position, const glm::vec3& scale,
        Crafting::IngredientType ingredientType, const std::string& displayName
    ){
        SceneNode* node = scene.CreateNode(parent, nodeName);

        node->AddObject<MeshRenderer>(mesh, material);

        node->LocalTransform().Position() = position;
        node->LocalTransform().Scale() = scale;

        auto* item = node->AddObject<Crafting::DraggableCraftingItem>();
        item->data = { ingredientType, displayName };

        node->AddObject<Physics::Body>(
            JPH::BodyCreationSettings{
                Physics::BoxShape(scale * 0.5f),
                JPH::RVec3::sZero(),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Static,
                Physics::Layers::NON_MOVING
            }
        );

        return item;
    }

    inline SceneNode* CreatePlayer(Scene& scene, SceneNode* parent, SceneNode* floorNode){
        JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
            new JPH::CharacterVirtualSettings();

        characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
        characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
        characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

        SceneNode* playerNode = scene.CreateNode("Player");
        playerNode->GlobalTransform().Position() = glm::vec3(-2.0f, 0.0f, 0.0f);

        SceneNode* bimberman = GltfImporter::LoadScene(
            &scene,
            "./res/models/bimbermann_throwing.glb",
            "Bimberman"
        );

        if (bimberman){
            bimberman->SetParent(playerNode);
        }else{
            spdlog::warn("CraftingScene: failed to load Bimberman model.");
        }

        auto* virtualCharacter =
            playerNode->AddObject<Physics::VirtualCharacterController>(
                characterSettings);

        virtualCharacter->SetPosition(
            playerNode->GlobalTransform().Position().Value());

        virtualCharacter->SetGravityFactor(0);
        virtualCharacter->SetCollisionLayerAndMask({0}, 0);

        auto* player = playerNode->AddObject<PlayerController>();

        auto* aimingAid =
            scene.CreateNode("AimingAid")->AddObject<AimingAid>();

        aimingAid->crosshair = GltfImporter::LoadScene(
            &scene,
            "./res/models/crosshair.glb",
            "crosshair",
            floorNode);

        if (aimingAid->crosshair){
            aimingAid->crosshair->SetParent(aimingAid->GetNode());
        }else{
            spdlog::warn("CraftingScene: failed to load crosshair.");
        }

        player->aim = aimingAid;

        player->AddObject<Player>();

        spdlog::info("CraftingScene: PlayerController player created.");

        return playerNode;
    }

    inline void InitScene(Scene& scene){
        spdlog::info("Initializing Crafting Scene");

        scene.AddComponent<Physics::System>();
        scene.AddComponent<LightSystem>();
        scene.AddComponent<UiSystem>();
        scene.AddComponent<AnimationSystem>();
        scene.AddComponent<TweenSystem>();

        SceneNode* rootNode = scene.CreateNode("Crafting Root");

        SceneNode* craftingRootNode = scene.CreateNode(rootNode, "Root");
        craftingRootNode->GlobalTransform().Position() = glm::vec3(0.0f, 0.0f, 0.0f);

        SceneNode* dragInteractorNode =
            scene.CreateNode(craftingRootNode, "Crafting Drag Interactor");

        dragInteractorNode->AddObject<Crafting::CraftingDragInteractor>();

        SceneNode* floorNode =
            GltfImporter::LoadScene(&scene, "./res/models/floor2804.glb", "Floor", rootNode);

        auto* floorMeshRenderer =
            floorNode->GetObjectInChildren<MeshRenderer>();

        if (floorMeshRenderer){
            floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
                JPH::BodyCreationSettings{
                    Physics::MeshShape(floorMeshRenderer->GetMesh()),
                    JPH::RVec3::sZero(),
                    JPH::Quat::sIdentity(),
                    JPH::EMotionType::Static,
                    Physics::Layers::NON_MOVING
                }
            );
        }else{
            spdlog::warn("CraftingScene: floor mesh renderer was not found.");
        }

        SceneNode* playerNode = CreatePlayer(scene, rootNode, floorNode);

        SceneNode* cameraNode = scene.CreateNode("Camera Node");

        cameraNode->AddObject<Camera>(
            Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));

        cameraNode->GetObject<Camera>()->SetAsMainCamera();

        auto* cameraSettings = cameraNode->AddObject<CameraSettings>(
            playerNode->GlobalTransform().Position());

        cameraSettings->height = 5.0f;
        cameraSettings->angleY = 0.0f;
        cameraSettings->angleX = 45.0f;
        cameraSettings->lerpAmount = 0.0f;

        cameraNode->AddObject<Tonemapper>()
            ->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);

        SceneNode* bimberMachineNode = GltfImporter::LoadScene(
            &scene,
            "./res/models/bimberMACHINAWIP.glb",
            "Bimber Machine",
            craftingRootNode
        );

        if (bimberMachineNode){
            bimberMachineNode->LocalTransform().Position() = glm::vec3(-4.0f, 0.0f, 0.0f);
            bimberMachineNode->LocalTransform().Rotation() =
                glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 0.0f)));
            bimberMachineNode->LocalTransform().Scale() = glm::vec3(1.0f);

            AddStaticPhysicsFromModel(bimberMachineNode);

            auto* craftingStation = bimberMachineNode->AddObject<Crafting::CraftingStation>();
            craftingStation->interactionRadius = 3.0f;
            craftingStation->stationCameraPosition = glm::vec3(4.0f, 2.0f, 0.0f);
            craftingStation->stationCameraRotation =
                glm::quat(glm::radians(glm::vec3(20.0f, -90.0f, 0.0f)));
        }else{
            spdlog::error("CraftingScene: failed to load bimberMachine");
        }

        Mesh* cubeMesh = scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

        Material* sugarMaterial =
            CreateColorMaterial(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        Material* waterMaterial =
            CreateColorMaterial(glm::vec4(0.2f, 0.45f, 1.0f, 1.0f));

        CreateDraggableCube(
            scene,
            craftingRootNode,
            "Sugar Ingredient",
            cubeMesh,
            sugarMaterial,
            glm::vec3(0.0f, 1.0f, 2.0f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            Crafting::IngredientType::Sugar,
            "Sugar"
        );

        CreateDraggableCube(
            scene,
            craftingRootNode,
            "Water Ingredient",
            cubeMesh,
            waterMaterial,
            glm::vec3(0.0f, 1.0f, -2.0f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            Crafting::IngredientType::Water,
            "Water"
        );

        SceneNode* sunNode = scene.CreateNode(rootNode, "Sun");

        auto* sun = sunNode->AddObject<Light>(
            Light::DirectionalLight(
                glm::vec3(1.0f, 1.0f, 1.0f),
                2.0f
            )
        );

        sun->SetShadowCasting(true);

        sunNode->GlobalTransform().Position() = glm::vec3(1.0f, 4.0f, 2.0f);
        sunNode->GlobalTransform().Rotation() =
            glm::quat(glm::radians(glm::vec3(50.0f, -20.0f, 0.0f)));

        if (auto* lightSystem = scene.GetComponent<LightSystem>()){
            lightSystem->SetAmbientLight(
                glm::vec4(1.0f, 1.0f, 1.0f, 0.6f)
            );
        }

        spdlog::info("Crafting Sandbox Scene initialized");
    }
}