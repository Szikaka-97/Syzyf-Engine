#pragma once

#include <GameObject.h>
#include <Scene.h>
#include <Mesh.h>
#include <Material.h>
#include <MeshRenderer.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <vector>

class ThrowBottle : public GameObject
{
private:
    struct BottleInstance {
        SceneNode* node = nullptr;
        bool active = false;

        glm::vec3 startPosition = glm::vec3(0.0f);
        glm::vec3 targetPosition = glm::vec3(0.0f);

        float flightDuration = 0.8f;
        float arcHeight = 3.0f;
        float timer = 0.0f;
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

            bottles.push_back(instance);
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

        for (auto& bottle : bottles) {
            if (!bottle.active) {
                bottle.active = true;
                bottle.startPosition = startPos;
                bottle.targetPosition = targetPos;
                bottle.flightDuration = duration;
                bottle.arcHeight = arc;
                bottle.timer = 0.0f;

                bottle.node->SetEnabled(true);
                bottle.node->GlobalTransform().Position() = startPos;
                bottle.node->GlobalTransform().Scale() = glm::vec3(0.3f);
                return;
            }
        }
    }

    void Update() {
        const float dt = 1.0f / 60.0f;

        for (auto& bottle : bottles) {
            if (!bottle.active || bottle.node == nullptr) {
                continue;
            }

            bottle.timer += dt;

            float t = std::clamp(bottle.timer / bottle.flightDuration, 0.0f, 1.0f);
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
            }
        }
    }
};