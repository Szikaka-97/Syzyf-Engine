#pragma once

#include "game_scripts/PickableItem.h"
#include <game_scripts/PotionInventory.h>

#include <GameObject.h>
#include <MeshRenderer.h>
#include <Shader.h>
#include <Material.h>
#include <GltfScene.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <physics/Body.h>
#include <Scene.h>
#include <Random.h>
#include <Formatters.h>

#include <glm/glm.hpp>
#include <string>

class LootItem : public PickableItem {
public:
    virtual ~LootItem() = default;
    virtual void Spawn(Scene* scene, const glm::vec3& position) const = 0;
};

inline SceneNode* SpawnIngredientLootModel(
    Scene* scene,
    const glm::vec3& position,
    const std::string& modelPath,
    const std::string& nodeName
){
    SceneNode* node = scene
        ->Resources()
        ->Get<GltfScene>(modelPath)
        ->Instantiate(scene,nullptr,nodeName);

    node->GlobalTransform().Position() = position;
    
    for (MeshRenderer* itemPart : node->GetAllObjectsInChildren<MeshRenderer>()) {
		// ratPart->maskFlags |= MaskEffectBits::Outline;
		for (int materialIndex = 0; materialIndex < itemPart->GetMaterialCount(); materialIndex++) {
			itemPart->GetMaterial(materialIndex)->SetValue("ambientBump", 0.5f);
		}
	}

    JPH::ShapeRefC lootShape =
        new JPH::BoxShape(JPH::Vec3(0.18f,0.18f,0.18f));

            JPH::BodyCreationSettings lootSettings(
                lootShape,
                JPH::RVec3(position.x,position.y,position.z),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Dynamic,
                Physics::Layers::MOVING
            );

    Physics::Body* lootBody = node->AddObject<Physics::Body>(lootSettings);
    lootBody->SetCollisionLayerAndMask({0},0xFFFFFFFF);

    glm::vec3 randomVel = Random::RandomOnUnitSphere();

    randomVel.y = glm::abs(randomVel.y);

    spdlog::info(randomVel);

    // lootBody->ApplyForce(randomVel * 100.0f);
    lootBody->ApplyImpulse(randomVel * 100.0f);

    return node;
}

class LootSugar : public LootItem {
public:
    void Spawn(Scene* scene, const glm::vec3& position) const override{
        SceneNode* node = SpawnIngredientLootModel(
            scene,
            position,
            "./res/models/ingredients/sugar.glb",
            "LootSugar"
        );

        node->AddObject<LootSugar>();
    }

    void OnPickUp() override{
        PotionInventory::AddIngredient(
            PotionInventory::IngredientSugarExplosionKey,
            1
        );
    }
};

class LootHoney : public LootItem {
public:
    void Spawn(Scene* scene, const glm::vec3& position) const override{
        SceneNode* node = SpawnIngredientLootModel(
            scene,
            position,
            "./res/models/ingredients/honey.glb",
            "LootHoney"
        );

        node->AddObject<LootHoney>();
    }

    void OnPickUp() override{
        PotionInventory::AddIngredient(
            PotionInventory::IngredientHoneyRadiusKey,
            1
        );
    }
};

class LootBone : public LootItem {
public:
    void Spawn(Scene* scene, const glm::vec3& position) const override{
        SceneNode* node = SpawnIngredientLootModel(
            scene,
            position,
            "./res/models/ingredients/bone.glb",
            "LootBone"
        );

        node->AddObject<LootBone>();
    }

    void OnPickUp() override{
        PotionInventory::AddIngredient(
            PotionInventory::IngredientBonePetrifyKey,
            1
        );
    }
};

class LootPotato : public LootItem {
    public:
        void Spawn(Scene* scene, const glm::vec3& position) const override{
            SceneNode* node = SpawnIngredientLootModel(
                scene,
                position,
                "./res/models/ingredients/dried_potato.glb",
                "LootDriedPotato"
            );

            node->AddObject<LootPotato>();
        }

        void OnPickUp() override{
            PotionInventory::AddIngredient(
                PotionInventory::IngredientDriedPotatoDurationKey,
                1
            );
        }
};

class LootBeetroot : public LootItem {
public:
    void Spawn(Scene* scene, const glm::vec3& position) const override{
        SceneNode* node = SpawnIngredientLootModel(
            scene,
            position,
            "./res/models/ingredients/dried_beet.glb",
            "LootDriedBeet"
        );

        node->AddObject<LootBeetroot>();
    }

    void OnPickUp() override{
        PotionInventory::AddIngredient(
            PotionInventory::IngredientDriedBeetBurnKey,
            1
        );
    }
};

class LootRatTail : public LootItem {
public:
    void Spawn(Scene* scene, const glm::vec3& position) const override{
        SceneNode* node = SpawnIngredientLootModel(
            scene,
            position,
            "./res/models/ingredients/rat_tail.glb",
            "LootRatTail"
        );

        node->AddObject<LootRatTail>();
    }

    void OnPickUp() override{
        PotionInventory::AddIngredient(
            PotionInventory::IngredientRatTailConfuseKey,
            1
        );
    }
};

class LootCrystal : public LootItem {
public:
    void Spawn(Scene* scene, const glm::vec3& position) const override{
        SceneNode* node = SpawnIngredientLootModel(
            scene,
            position,
            "./res/models/ingredients/water.glb",
            "LootWater"
        );

        node->AddObject<LootCrystal>();
    }

    void OnPickUp() override{
        PotionInventory::AddIngredient(
            PotionInventory::IngredientWaterTornadoKey,
            1
        );
    }
};

class LootDeserterEar : public LootItem {
public:
    void Spawn(Scene* scene, const glm::vec3& position) const override{
        SceneNode* node = SpawnIngredientLootModel(
            scene,
            position,
            "./res/models/ingredients/deserter_ear.glb",
            "LootDeserterEar"
        );

        node->AddObject<LootDeserterEar>();
    }

    void OnPickUp() override{
        PotionInventory::AddIngredient(
            PotionInventory::IngredientDeserterEarPowerKey,
            1
        );
    }
};