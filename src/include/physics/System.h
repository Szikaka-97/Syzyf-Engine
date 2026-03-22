#pragma once

#include <Jolt/Jolt.h>
#include "Jolt/Core/Core.h"
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <glm/fwd.hpp>

#include "Jolt/Physics/Body/BodyFilter.h"
#include "SceneComponent.h"

class SceneNode;

namespace JPH {
  class BroadPhaseLayerInterface;
  class ObjectVsBroadPhaseLayerFilter;
  class ObjectLayerPairFilter;
  class BodyInterface;
  class ContactListener;
  class BodyActivationListener;
}

namespace Physics {

struct SystemSettings {
  JPH::uint maxBodies = 1024;
  JPH::uint numBodyMutexes = 0;
  JPH::uint maxBodyPairs = 1024;
  JPH::uint maxContactConstraints = 1024;
  JPH::uint tempAllocatorSize = 10 * 1024 * 1024;
};

struct CollisionData {
  JPH::BodyID body1;
  JPH::BodyID body2;
  enum class State { Enter, Exit } state;
};

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

class System : public SceneComponent {
public:
  // move to private perhaps
  std::mutex collisionMutex;
  std::vector<CollisionData> collisionQueue;
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

  JPH::ContactListener* contactListener = nullptr;
  JPH::BodyActivationListener* bodyActivationListener = nullptr;

  JPH::GroupFilter* layerGroupFilter = nullptr;
public:
  System(Scene* scene, const SystemSettings& settings = SystemSettings());
  virtual ~System();


  void OnPreUpdate();
  void OnPostRender();

  void DrawImGui();

public:
  void OptimizeBroadPhase();

  SceneNode* CastRay(
    glm::vec3 origin,
    glm::vec3 direction,
    const JPH::BroadPhaseLayerFilter& broadPhaseLayerFilter = {},
    const JPH::ObjectLayerFilter& objectLayerFilter = {},
    const JPH::BodyFilter& bodyFilter = {}
  );

  std::vector<SceneNode*> CastShape(
    glm::vec3 origin,
    glm::vec3 direction,
    const JPH::ShapeRefC& shape,
    const JPH::BroadPhaseLayerFilter& broadPhaseLayerFilter = {},
    const JPH::ObjectLayerFilter& objectLayerFilter = {},
    const JPH::BodyFilter& bodyFilter = {}
  );

  glm::vec3 GetGravity() const;
  void SetGravity(const glm::vec3 gravity);

  JPH::BodyInterface& GetBodyInterface();
  JPH::PhysicsSystem& GetSystem();
  JPH::TempAllocatorImpl& GetTempAllocator();

  JPH::GroupFilter* GetLayerGroupFilter() const;
};
}
