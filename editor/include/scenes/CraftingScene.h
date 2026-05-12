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

#include "game_scripts/crafting/CraftingDragInteractor.h"
#include "game_scripts/crafting/DraggableCraftingItem.h"

#include <physics/Body.h>
#include <physics/Helpers.h>
#include <physics/System.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>

#include <glm/glm.hpp>
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

    inline void InitScene(Scene& scene){
        spdlog::info("Initializing Crafting Scene");

        scene.AddComponent<Physics::System>();
        scene.AddComponent<LightSystem>();

        SceneNode* rootNode = scene.CreateNode("Crafting Root");

        SceneNode* craftingRootNode = scene.CreateNode(rootNode, "Root");
        craftingRootNode->GlobalTransform().Position() = glm::vec3(0.0f, 0.0f, 0.0f);

        SceneNode* cameraNode = scene.CreateNode(rootNode, "Crafting Camera");

        auto* camera = cameraNode->AddObject<Camera>(
            Camera::Perspective(
                60.0f,
                16.0f / 9.0f,
                0.1f,
                200.0f
            )
        );

        camera->SetAsMainCamera();

        cameraNode->GlobalTransform().Position() = glm::vec3(4.0f, 2.0f, 0.0f);
        cameraNode->GlobalTransform().Rotation() =
            glm::quat(glm::radians(glm::vec3(20.0f, -90.0f, 0.0f)));

        cameraNode->AddObject<Tonemapper>()
            ->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);

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