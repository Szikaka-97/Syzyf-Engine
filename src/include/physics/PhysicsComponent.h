#pragma once

#include <Jolt/Jolt.h>
#include "Jolt/Core/Core.h"
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <glm/fwd.hpp>

#include "Jolt/Math/Vec3.h"
#include "SceneComponent.h"

namespace JPH {
    class BroadPhaseLayerInterface;
    class ObjectVsBroadPhaseLayerFilter;
    class ObjectLayerPairFilter;
    class BodyInterface;
    class ContactListener;
    class BodyActivationListener;
}

class PhysicsComponent : public SceneComponent {
public:
  PhysicsComponent(Scene* scene);
  virtual ~PhysicsComponent();

  struct Layers {
      static constexpr JPH::ObjectLayer NON_MOVING = 0;
      static constexpr JPH::ObjectLayer MOVING = 1;
      static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
  };

  struct BroadPhaseLayers {
      static constexpr JPH::BroadPhaseLayer NON_MOVING{0};
      static constexpr JPH::BroadPhaseLayer MOVING{1};
      static constexpr JPH::uint NUM_LAYERS{2};
  };
private:
    bool drawDebug = false;

    float accumulator = 0.0f;
    const float cDeltaTime = 1.0f / 60.0f;

    JPH::PhysicsSystem* physicsSystem = nullptr;
    JPH::TempAllocatorImpl* tempAllocator = nullptr;
    JPH::JobSystemThreadPool* jobSystem = nullptr;

    JPH::BroadPhaseLayerInterface* bpLayerInterface = nullptr;
    JPH::ObjectVsBroadPhaseLayerFilter* objVsBPFilter = nullptr;
    JPH::ObjectLayerPairFilter* objVsObjFilter = nullptr;
    JPH::BodyInterface* bodyInterface = nullptr;
public:
  void OnPostUpdate();
  void OnPostRender();

  void DrawImGui();

  glm::vec3 GetGravity() const;
  void SetGravity(const glm::vec3 gravity);

  JPH::BodyInterface& GetBodyInterface();
  JPH::PhysicsSystem& GetSystem();
private:
};
