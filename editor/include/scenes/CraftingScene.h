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
#include "game_scripts/crafting/CraftingInteractable.h"
#include "game_scripts/crafting/DraggableCraftingItem.h"
#include "game_scripts/crafting/CraftingStation.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"
#include "game_scripts/crafting/Cauldron.h"

#include <Player.h>
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

#include <string>
#include <vector>

namespace CraftingScene{
    struct IngredientSpawnData{
        std::string nodeName;
        Material* material = nullptr;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        Crafting::IngredientData ingredientData;
    };

    inline SceneNode* CreateLocalPoint(
        Scene& scene,
        SceneNode* parent,
        const std::string& nodeName,
        const glm::vec3& localPosition
    ){
        SceneNode* node = scene.CreateNode(parent,nodeName);

        node->LocalTransform().Position() = localPosition;

        return node;
    }

    inline void CreateCraftingStageCameraPoints(Scene& scene, SceneNode* machineNode){
        if (!machineNode){
            return;
        }

        CreateLocalPoint(
            scene,
            machineNode,
            "IngredientCameraPoint",
            glm::vec3(1.5f, 3.5f, 1.5f)
        );

        CreateLocalPoint(
            scene,
            machineNode,
            "IngredientCameraTarget",
            glm::vec3(0.0f, 1.0f, 1.5f)
        );

        CreateLocalPoint(
            scene,
            machineNode,
            "HeatingCameraPoint",
            glm::vec3(1.5f, 0.5f, 1.5f)
        );

        CreateLocalPoint(
            scene,
            machineNode,
            "HeatingCameraTarget",
            glm::vec3(0.0f, 0.5f, 1.5f)
        );

        CreateLocalPoint(
            scene,
            machineNode,
            "BottlingCameraPoint",
            glm::vec3(0.0f, 0.5f, -5.0f)
        );

        CreateLocalPoint(
            scene,
            machineNode,
            "BottlingCameraTarget",
            glm::vec3(0.0f, 0.5f, 1.5f)
        );
    }

    inline SceneNode* CreateStationHitbox(Scene& scene, SceneNode* machineNode){
        if (!machineNode){
            return nullptr;
        }

        SceneNode* stationHitboxNode =
            scene.CreateNode(machineNode, "StationHitbox");

        stationHitboxNode->LocalTransform().Position() =
            glm::vec3(0.0f, 4.5f, 0.0f);

        stationHitboxNode->LocalTransform().Scale() =
            glm::vec3(1.0f);

        glm::vec3 stationHitboxPosition =
            stationHitboxNode->GlobalTransform().Position().Value();

        auto* stationBody = stationHitboxNode->AddObject<Physics::Body>(
            JPH::BodyCreationSettings{
                Physics::BoxShape(glm::vec3(0.6f, 1.5f, 3.6f)),
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

        spdlog::info(
            "CraftingScene: StationHitbox created at {} {} {}.",
            stationHitboxPosition.x,
            stationHitboxPosition.y,
            stationHitboxPosition.z
        );

        return stationHitboxNode;
    }

    inline Material* CreateColorMaterial(const glm::vec4& color);

    inline SceneNode* CreateBlowerHitbox(Scene& scene, SceneNode* machineNode){
        if (!machineNode){
            return nullptr;
        }

        SceneNode* firePlaceNode =
            machineNode->FindNode("Fire_Place");

        if (!firePlaceNode){
            spdlog::warn(
                "CraftingScene: Fire_Place node not found. BlowerHitbox will be attached to machine root."
            );

            firePlaceNode = machineNode;
        }

        SceneNode* blowerHitboxNode =
            scene.CreateNode(firePlaceNode, "BlowerHitbox");

        blowerHitboxNode->LocalTransform().Position() =
            glm::vec3(0.0f, 0.15f, 0.0f);

        blowerHitboxNode->LocalTransform().Scale() =
            glm::vec3(0.45f, 0.45f, 0.45f);

        Mesh* cubeMesh =
            scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

        Material* blowerMaterial =
            CreateColorMaterial(glm::vec4(0.85f, 0.2f, 1.0f, 0.65f));

        if (cubeMesh && blowerMaterial){
            blowerHitboxNode->AddObject<MeshRenderer>(
                cubeMesh,
                blowerMaterial
            );
        }else{
            spdlog::warn("CraftingScene: failed to create visible BlowerHitbox cube.");
        }

        glm::vec3 blowerHitboxPosition =
            blowerHitboxNode->GlobalTransform().Position().Value();

        auto* blowerBody = blowerHitboxNode->AddObject<Physics::Body>(
            JPH::BodyCreationSettings{
                Physics::BoxShape(glm::vec3(0.25f, 0.25f, 0.25f)),
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

        spdlog::info(
            "CraftingScene: visible BlowerHitbox cube created at {} {} {}.",
            blowerHitboxPosition.x,
            blowerHitboxPosition.y,
            blowerHitboxPosition.z
        );

        return blowerHitboxNode;
    }

    inline SceneNode* CreateDoorHitbox(Scene& scene, SceneNode* machineNode){
        if (!machineNode){
            return nullptr;
        }

        SceneNode* doorNode =
            machineNode->FindNode("Door");

        if (!doorNode){
            spdlog::warn("CraftingScene: Door node not found. DoorHitbox was not created.");
            return nullptr;
        }

        SceneNode* doorHitboxNode =
            scene.CreateNode(doorNode, "DoorHitbox");

        doorHitboxNode->LocalTransform().Position() =
            glm::vec3(0.0f, 0.0f, 0.0f);

        doorHitboxNode->LocalTransform().Scale() =
            glm::vec3(0.7f, 0.8f, 0.35f);

        Mesh* cubeMesh =
            scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

        Material* doorMaterial =
            CreateColorMaterial(glm::vec4(0.2f, 0.7f, 1.0f, 0.55f));

        if (cubeMesh && doorMaterial){
            doorHitboxNode->AddObject<MeshRenderer>(
                cubeMesh,
                doorMaterial
            );
        }else{
            spdlog::warn("CraftingScene: failed to create visible DoorHitbox cube.");
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

        spdlog::info(
            "CraftingScene: visible DoorHitbox cube created at {} {} {}.",
            doorHitboxPosition.x,
            doorHitboxPosition.y,
            doorHitboxPosition.z
        );

        return doorHitboxNode;
    }


    inline SceneNode* CreateValveHitbox(Scene& scene, SceneNode* machineNode){
        if (!machineNode){
            return nullptr;
        }

        SceneNode* valveNode = machineNode->FindNode("Knob_One.001");
        bool valveAttachedToMachineRoot = false;

        if (!valveNode){
            valveNode = machineNode->FindNode("Knob_One");
        }

        if (!valveNode){
            spdlog::warn(
                "CraftingScene: Knob_One.001 node not found. ValveHitbox will be attached to machine root fallback position."
            );

            valveNode = machineNode;
            valveAttachedToMachineRoot = true;
        }

        SceneNode* valveHitboxNode =
            scene.CreateNode(valveNode, "ValveHitbox");

        valveHitboxNode->LocalTransform().Position() =
            valveAttachedToMachineRoot
                ? glm::vec3(0.0f, 0.75f, 1.5f)
                : glm::vec3(0.0f, 0.0f, 0.0f);

        valveHitboxNode->LocalTransform().Scale() =
            glm::vec3(0.35f, 0.35f, 0.35f);

        Mesh* cubeMesh =
            scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

        Material* valveMaterial =
            CreateColorMaterial(glm::vec4(1.0f, 0.85f, 0.15f, 0.65f));

        if (cubeMesh && valveMaterial){
            valveHitboxNode->AddObject<MeshRenderer>(
                cubeMesh,
                valveMaterial
            );
        }else{
            spdlog::warn("CraftingScene: failed to create visible ValveHitbox cube.");
        }

        glm::vec3 valveHitboxPosition =
            valveHitboxNode->GlobalTransform().Position().Value();

        auto* valveBody = valveHitboxNode->AddObject<Physics::Body>(
            JPH::BodyCreationSettings{
                Physics::BoxShape(glm::vec3(0.3f, 0.3f, 0.3f)),
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

        spdlog::info(
            "CraftingScene: visible ValveHitbox cube created at {} {} {}.",
            valveHitboxPosition.x,
            valveHitboxPosition.y,
            valveHitboxPosition.z
        );

        return valveHitboxNode;
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


    inline SceneNode* CreateBottlingDebugCube(
        Scene& scene,
        SceneNode* parent,
        const std::string& nodeName,
        Mesh* mesh,
        Material* material,
        const glm::vec3& localPosition,
        const glm::vec3& localScale
    ){
        SceneNode* node = scene.CreateNode(parent,nodeName);

        node->LocalTransform().Position() = localPosition;
        node->LocalTransform().Scale() = localScale;

        if (mesh && material){
            node->AddObject<MeshRenderer>(mesh,material);
        }

        return node;
    }

inline void CreateBottlingStageNodes(Scene& scene, SceneNode* machineNode){
    if (!machineNode){
        return;
    }

    const glm::vec3 bottleStartPoint =
        glm::vec3(-2.624f, -0.113f, -3.330f);

    const glm::vec3 bottleFillPoint =
        glm::vec3(-4.009f, -0.103f, -3.357f);

    const glm::vec3 bottleEndPoint =
        glm::vec3(-5.227f, -0.075f, -3.330f);

    const glm::vec3 lanePosition =
        glm::vec3(-4.000f, -0.213f, -3.330f);

    const glm::vec3 laneScale =
        glm::vec3(2.850f, 0.080f, 0.450f);

    const glm::vec3 fillZonePosition =
        glm::vec3(-4.009f, -0.103f, -3.357f);

    const glm::vec3 fillZoneScale =
        glm::vec3(0.340f, 0.550f, 0.340f);

    const glm::vec3 valveGuidePosition =
        glm::vec3(-4.000f, 0.189f, -3.330f);

    const glm::vec3 valveGuideScale =
        glm::vec3(0.080f, 0.420f, 0.080f);

    const glm::vec3 bottleScale =
        glm::vec3(0.180f, 0.450f, 0.180f);

    CreateLocalPoint(
        scene,
        machineNode,
        "BottleStartPoint",
        bottleStartPoint
    );

    CreateLocalPoint(
        scene,
        machineNode,
        "BottleFillPoint",
        bottleFillPoint
    );

    CreateLocalPoint(
        scene,
        machineNode,
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
        scene.CreateNode(machineNode,"BottlingBottlesRoot");

    bottlesRoot->LocalTransform().Position() = glm::vec3(0.0f);

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
        fillZonePosition,
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

    for (int i = 0; i < 4; ++i){
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
            glm::vec3(0.110f, 0.230f, 0.110f)
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
        ){
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
        ){
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
    ){
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
                Physics::BoxShape(scale * 1.0f),
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
            "./res/models/bimbermanMachineNameingScheme.glb",
            "Bimber Machine",
            craftingRootNode
        );

        if (bimberMachineNode){
            bimberMachineNode->LocalTransform().Position() =
                glm::vec3(-4.0f, 0.0f, 0.0f);

            bimberMachineNode->LocalTransform().Rotation() =
                glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 0.0f)));

            bimberMachineNode->LocalTransform().Scale() =
                glm::vec3(1.0f);

            CreateCraftingStageCameraPoints(scene,bimberMachineNode);
            CreateStationHitbox(scene,bimberMachineNode);
            CreateBlowerHitbox(scene,bimberMachineNode);
            CreateDoorHitbox(scene,bimberMachineNode);
            CreateValveHitbox(scene,bimberMachineNode);
            CreateBottlingStageNodes(scene,bimberMachineNode);

            SceneNode* cauldronNode = bimberMachineNode->FindNode("Cauldron");

            if (cauldronNode){
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

                spdlog::info(
                    "CraftingScene: CauldronReceiverHitbox created at {} {} {}.",
                    cauldronReceiverHitboxPosition.x,
                    cauldronReceiverHitboxPosition.y,
                    cauldronReceiverHitboxPosition.z
                );
            }else{
                spdlog::error("CraftingScene: Cauldron not found.");
            }

            SceneNode* lidNode = bimberMachineNode->FindNode("Lid");

            if (lidNode){
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

                spdlog::info(
                    "CraftingScene: LidHitbox created at {} {} {}.",
                    lidHitboxPosition.x,
                    lidHitboxPosition.y,
                    lidHitboxPosition.z
                );
            }else{
                spdlog::error("CraftingScene: Lid not found.");
            }

            auto* craftingStation =
                bimberMachineNode->AddObject<Crafting::CraftingStation>();

            craftingStation->interactionRadius = 3.0f;
            craftingStation->stationCameraPosition =
                glm::vec3(0.0f, 5.0f, 0.0f);

            craftingStation->stationCameraRotation =
                glm::quat(glm::radians(glm::vec3(60.0f, -90.0f, 0.0f)));
        }else{
            spdlog::error("CraftingScene: failed to load bimberMachine");
        }

        Mesh* cubeMesh = scene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

        const glm::vec4 burnColor = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);
        const glm::vec4 lightningColor = glm::vec4(1.0f, 1.0f, 0.1f, 1.0f);
        const glm::vec4 radiusColor = glm::vec4(0.1f, 0.8f, 0.2f, 1.0f);
        const glm::vec4 durationColor = glm::vec4(0.1f, 0.3f, 1.0f, 1.0f);

        Material* burnMaterial = CreateColorMaterial(burnColor);
        Material* lightningMaterial = CreateColorMaterial(lightningColor);
        Material* radiusMaterial = CreateColorMaterial(radiusColor);
        Material* durationMaterial = CreateColorMaterial(durationColor);

        SceneNode* ingredientsRootNode =
            scene.CreateNode(craftingRootNode, "Crafting Ingredients");

        const std::vector<IngredientSpawnData> ingredientSpawns = {
            {
                "Burn Ingredient",
                burnMaterial,
                glm::vec3(-3.0f, 2.0f, 3.0f),
                glm::vec3(0.35f, 0.35f, 0.35f),
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
                glm::vec3(-3.0f, 2.0f, 2.2f),
                glm::vec3(0.35f, 0.35f, 0.35f),
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
                glm::vec3(-3.0f, 2.0f, 1.4f),
                glm::vec3(0.35f, 0.35f, 0.35f),
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
                glm::vec3(-3.0f, 2.0f, 0.6f),
                glm::vec3(0.35f, 0.35f, 0.35f),
                CreateModifierIngredient(
                    Crafting::IngredientType::Sugar,
                    "Duration",
                    Crafting::ModifierId::Duration,
                    1.0f,
                    durationColor
                )
            }
        };

        for (const IngredientSpawnData& spawn : ingredientSpawns){
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

        SceneNode* sunNode = scene.CreateNode(rootNode, "Sun");

        auto* sun = sunNode->AddObject<Light>(
            Light::DirectionalLight(
                glm::vec3(1.0f, 1.0f, 1.0f),
                2.0f
            )
        );

        sun->SetShadowCasting(true);

        sunNode->GlobalTransform().Position() =
            glm::vec3(1.0f, 4.0f, 2.0f);

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