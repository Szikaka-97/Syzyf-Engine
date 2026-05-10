#pragma once

#include "GltfImporter.h"
#include "Light.h"
#include "LightSystem.h"
#include "MeshRenderer.h"
#include "Scene.h"

#include <physics/Body.h>
#include <physics/Helpers.h>
#include <physics/System.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spdlog/spdlog.h>

namespace CraftingScene{
    inline void AddStaticPhysicsFromModel(SceneNode* modelNode){
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

    inline void InitScene(Scene& scene){
        spdlog::info("Initializing Crafting Scene");

        scene.AddComponent<Physics::System>();
        scene.AddComponent<LightSystem>();

        SceneNode* rootNode = scene.CreateNode("Crafting Root");

        SceneNode* craftingRootNode = scene.CreateNode(rootNode, "Root");
        craftingRootNode->GlobalTransform().Position() = glm::vec3(0.0f, 0.0f, 0.0f);

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
            bimberMachineNode->LocalTransform().Position() = glm::vec3(0.0f, 0.0f, 0.0f);
            bimberMachineNode->LocalTransform().Rotation() =
                glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 0.0f)));
            bimberMachineNode->LocalTransform().Scale() = glm::vec3(1.0f);

            AddStaticPhysicsFromModel(bimberMachineNode);
        }else{
            spdlog::error("CraftingScene: failed to load bimberMachine");
        }

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