#pragma once

#include <physics/System.h>
#include <physics/ICollisionReceiver.h>
#include <GameObject.h>
#include <Scene.h>
#include <Mesh.h>
#include <Material.h>
#include <MeshRenderer.h>
#include <AiNode.h>

#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <vector>
#include <TimeSystem.h>

class Bottle : public GameObject, public Physics::ICollisionReceiver {
public:
    std::function<void(SceneNode*)> onCollision;

    void OnCollisionEnter(SceneNode* other) override {
        if (onCollision) onCollision(other);
    }
    void OnCollisionExit(SceneNode* other) override {}
};

class ThrowBottle : public GameObject 
{
private:
    struct BottleInstance {
        SceneNode* node = nullptr;
        bool active = false;
        Physics::Body* body = nullptr;
       // glm::vec3 startPosition = glm::vec3(0.0f);
        //glm::vec3 targetPosition = glm::vec3(0.0f);

        //float flightDuration = 0.8f;
      //  float arcHeight = 3.0f;
        float timer = 0.0f;
        const float maxLifetime = 5.0f;
    };

    std::vector<BottleInstance> bottles;

    Mesh* bottleMesh = nullptr;
    Material* bottleMaterial = nullptr;

    int poolSize = 16;
    glm::vec3 hiddenPosition = glm::vec3(0.0f, -1000.0f, 0.0f);

    void CreatePoolIfNeeded()
    {
        if (!bottles.empty() || !bottleMesh || !bottleMaterial) {
            return;
        }

        bottles.reserve(poolSize);

        for (int i = 0; i < poolSize; ++i) {
            SceneNode* bottleNode = GetScene()->CreateNode("Thrown Bottle");
            bottleNode->AddObject<MeshRenderer>(bottleMesh, bottleMaterial);
            bottleNode->GlobalTransform().Scale() = glm::vec3(0.3f);
            bottleNode->GlobalTransform().Position() = hiddenPosition;
            bottleNode->SetEnabled(false);

            BottleInstance instance;
            instance.node = bottleNode;
            instance.body = nullptr;
            instance.active = false;
            bottles.push_back(instance);
        }
    }

    void DisableBottle(BottleInstance& bottle) {
        if (bottle.body) {
            bottle.node->DeleteObject(bottle.body);
            bottle.body = nullptr;
        }
        bottle.active = false;
        bottle.node->SetEnabled(false);
        bottle.node->GlobalTransform().Position() = hiddenPosition;
        bottle.timer = 0.0f;
    }

    void HandleBottleCollision(int index, SceneNode* other) {
        if (index < 0 || index >= bottles.size()) return;
        auto& bottle = bottles[index];
        if (!bottle.active) return;

        if (other->GetName() == "Floor") {
            DisableBottle(bottle);
        }
        else if (AiNode* ai = other->GetObject<AiNode>()) {
            ai->TakeDamage(25);
            DisableBottle(bottle);
        }
    }

public:
    ThrowBottle() = default;

    void SetResources(Mesh* mesh, Material* material)
    {
        bottleMesh = mesh;
        bottleMaterial = material;
        CreatePoolIfNeeded();
    }

    void SetPoolSize(int size)
    {
        if (size > 0 && bottles.empty()) {
            poolSize = size;
        }
    }

    void LaunchBottle(const glm::vec3& startPos, const glm::vec3& targetPos,
                      float duration = 0.8f, float arc = 3.0f)
    {
        CreatePoolIfNeeded();

        for (int i = 0; i < bottles.size(); ++i) {
            auto& bottle = bottles[i];
            if (!bottle.active) {
                bottle.active = true;
                //bottle.startPosition = startPos;
                //bottle.targetPosition = targetPos;
               // bottle.flightDuration = duration;
              //  bottle.arcHeight = arc;
                bottle.timer = 0.0f;

                bottle.node->SetEnabled(true);
                bottle.node->GlobalTransform().Position() = startPos;
                bottle.node->GlobalTransform().Scale() = glm::vec3(0.3f);

               JPH::BodyCreationSettings bottleSettings(
                    new JPH::SphereShape(0.2f),
                    JPH::RVec3(startPos.x, startPos.y, startPos.z),
                    JPH::Quat::sIdentity(),
                    JPH::EMotionType::Dynamic,
                    Physics::Layers::MOVING
                );
                bottle.body = bottle.node->AddObject<Physics::Body>(bottleSettings);
                bottle.body->SetRestitution(0.3f);
                bottle.body->SetFriction(0.5f);
                bottle.body->SetCollisionLayerAndMask({2}, {0});

                float gravity = 9.81f;
                glm::vec3 displacement = targetPos - startPos;
                float verticalVel = (displacement.y + 0.5f * gravity * duration * duration) / duration;
                glm::vec3 flatDispl(displacement.x, 0.0f, displacement.z);
                float flatDist = glm::length(flatDispl);
                glm::vec3 horizontalVel = flatDist > 0.001f ? glm::normalize(flatDispl) * (flatDist / duration) : glm::vec3(0.0f);
                glm::vec3 velocity = horizontalVel + glm::vec3(0.0f, verticalVel, 0.0f);

                bottle.body->SetLinearVelocity(velocity);
                //bottle.body->Awake();
                auto* bottleComp = bottle.node->AddObject<Bottle>();
                bottleComp->onCollision = [this, i](SceneNode* other) {
                    HandleBottleCollision(i, other);
                };
                return;
              }
        }
    }

    void Update() {
        //const float dt = 1.0f / 60.0f;
        const float dt = Time::Delta();
        for (auto& bottle : bottles) {
            if (!bottle.active || bottle.node == nullptr) {
                continue;
            }

            bottle.timer += dt;

           /* float t = std::clamp(bottle.timer / bottle.flightDuration, 0.0f, 1.0f);
            glm::vec3 pos = glm::mix(bottle.startPosition, bottle.targetPosition, t);

            float arc = 4.0f * bottle.arcHeight * t * (1.0f - t);
            pos.y += arc;

            bottle.node->GlobalTransform().Position() = pos;
            bottle.node->GlobalTransform().Rotation() *=
                glm::angleAxis(glm::radians(12.0f), glm::vec3(1, 0, 0));

            if (t >= 1.0f || pos.y <= 0.0f) {
                bottle.active = false;
                bottle.node->SetEnabled(false);
                bottle.node->GlobalTransform().Position() = hiddenPosition;
            }*/
            if (bottle.timer > bottle.maxLifetime) {
                DisableBottle(bottle);
                continue;
            }
            if (bottle.body) {
                bottle.node->GlobalTransform().Position() = bottle.body->GetPosition();
                bottle.node->GlobalTransform().Rotation() *= glm::angleAxis(glm::radians(360.0f * dt), glm::vec3(1,0,0));
            }
            if (bottle.node->GlobalTransform().Position().y < -5.0f) {
                DisableBottle(bottle);
            }
        }
    }
};
