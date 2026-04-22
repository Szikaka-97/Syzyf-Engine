#pragma once

#include "Light.h"
#include "MeshRenderer.h"
#include "ReflectionProbe.h"
#include "fog/Fog.h"
#include "fog/FogVolume.h"
#include "fog/VolumetricFog.h"
#include "physics/Body.h"
#include "physics/Helpers.h"
#include "physics/System.h"

#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/MotionType.h"
#include <functional>
#include <map>
#include <string>

class SceneNode;

namespace Editor {
using ComponentFactoryFunc = std::function<void(SceneNode*)>;

class ComponentRegistry {
  public:
    static ComponentRegistry& Get() {
        static ComponentRegistry instance;
        return instance;
    }

    void Register(const std::string& name, ComponentFactoryFunc func) {
        factories[name] = func;
    }

    const std::map<std::string, ComponentFactoryFunc>& GetFactories() const {
        return this->factories;
    }

    static void RegisterComponents() {
        auto& registry = ComponentRegistry::Get();

        registry.Register("Light", [](SceneNode* node) {
            node->AddObject<Light>(Light::PointLight({1, 1, 1}, 1, 1));
        });
        registry.Register("Reflection Probe", [](SceneNode* node) {
            node->AddObject<ReflectionProbe>();
        });
        // registry.Register("Particle Spawner", [](SceneNode* node) {
        //     node->AddObject<ParticleSpawner>(ParticleSpawnerSettings{});
        // });

        // Fog
        registry.Register("Fog Volume", [](SceneNode* node) {
            node->AddObject<FogVolume>();
        });
        registry.Register("Volumetric Fog", [](SceneNode* node) {
            node->AddObject<VolumetricFog>();
        });
        registry.Register("Fog",
                          [](SceneNode* node) { node->AddObject<Fog>(); });

        // Physics
        registry.Register("Physics Body", [](SceneNode* node) {
            if (node->GetObjectInChildren<MeshRenderer>() ||
                node->GetObject<MeshRenderer>()) {

                auto shape = JPH::BodyCreationSettings(
                    Physics::CreateCompoundShapeFromNode(
                        node, false, JPH::EMotionType::Static,
                        Physics::Layers::NON_MOVING),
                    JPH::RVec3::sZero(), JPH::Quat::sIdentity(),
                    JPH::EMotionType::Static, Physics::Layers::NON_MOVING);

                node->AddObject<Physics::Body>(shape);
            }
        });
    }

  private:
    std::map<std::string, ComponentFactoryFunc> factories;
};
} // namespace Editor
