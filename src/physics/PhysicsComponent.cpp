#include "physics/PhysicsComponent.h"

#include "GameObject.h"
#include "physics/PhysicsCharacter.h"
#include "physics/PhysicsCollisionReceiver.h"
#include "physics/PhysicsDebugRenderer.h"
#include "physics/PhysicsObject.h"
#include "physics/PhysicsContactListener.h"

#include "TimeSystem.h"
#include "Scene.h"



#include <Jolt/Core/Core.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Math/MathTypes.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <imgui.h>

using namespace JPH;
using namespace JPH::literals;

namespace {
  class GroupFilterLayerMask : public JPH::GroupFilter {
  public:
    virtual bool CanCollide(const JPH::CollisionGroup& inGroup1, const JPH::CollisionGroup& inGroup2) const override {
      return ((inGroup1.GetGroupID() & inGroup2.GetSubGroupID()) != 0) ||
             ((inGroup2.GetGroupID() & inGroup1.GetSubGroupID()) != 0);
    }
  };
  
  // Class that determines if two object layers can collide
  class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
  public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
      switch (inObject1) {
      case PhysicsComponent::Layers::NON_MOVING:
        return inObject2 == PhysicsComponent::Layers::MOVING; // Non moving only collides with moving
      case PhysicsComponent::Layers::MOVING:
  return true; // Moving collides with everything
      default:
        JPH_ASSERT(false);
        return false;
      }
    }
  };

  // BroadPhaseLayerInterface implementation
  // This defines a mapping between object and broadphase layers.
  class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
  public:
    BPLayerInterfaceImpl() {
      // Create a mapping table from object to broad phase layer
      mObjectToBroadPhase[PhysicsComponent::Layers::NON_MOVING] = PhysicsComponent::BroadPhaseLayers::NON_MOVING;
      mObjectToBroadPhase[PhysicsComponent::Layers::MOVING] = PhysicsComponent::BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override {
      return PhysicsComponent::BroadPhaseLayers::NUM_LAYERS;
    }

    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override{
      JPH_ASSERT(inLayer < PhysicsComponent::Layers::NUM_LAYERS);
      return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override{
      switch ((BroadPhaseLayer::Type)inLayer) {
      case (BroadPhaseLayer::Type)PhysicsComponent::BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
      case (BroadPhaseLayer::Type)PhysicsComponent::BroadPhaseLayers::MOVING: return "MOVING";
      default: JPH_ASSERT(false); return "INVALID";
      }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

  private:
    BroadPhaseLayer	mObjectToBroadPhase[PhysicsComponent::Layers::NUM_LAYERS];
  };

  // Class that determines if an object layer can collide with a broadphase layer
  class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
  public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
      switch (inLayer1) {
      case PhysicsComponent::Layers::NON_MOVING:
        return inLayer2 == PhysicsComponent::BroadPhaseLayers::MOVING;
      case PhysicsComponent::Layers::MOVING:
        return true;
      default:
        JPH_ASSERT(false);
        return false;
      }
    }
  };
};

  PhysicsComponent::PhysicsComponent(Scene* scene, const PhysicsSystemSettings& settings): SceneComponent(scene) {
    layerGroupFilter = new GroupFilterLayerMask();
    
    tempAllocator = new TempAllocatorImpl(settings.tempAllocatorSize);
    jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

    bpLayerInterface = new BPLayerInterfaceImpl();
    objVsBPFilter = new ObjectVsBroadPhaseLayerFilterImpl();
    objVsObjFilter = new ObjectLayerPairFilterImpl();

    physicsSystem = new PhysicsSystem();
    physicsSystem->Init(settings.maxBodies, settings.numBodyMutexes, settings.maxBodyPairs, settings.maxContactConstraints, *bpLayerInterface, *objVsBPFilter, *objVsObjFilter);
    bodyInterface = &physicsSystem->GetBodyInterface();

    contactListener = new PhysicsContactListener(this);
    physicsSystem->SetContactListener(contactListener);
    physicsSystem->SetBodyActivationListener(bodyActivationListener);
  }

  PhysicsComponent::~PhysicsComponent() {
    delete layerGroupFilter;
    delete contactListener;
    delete bodyActivationListener;
    delete physicsSystem;
    delete objVsObjFilter;
    delete objVsBPFilter;
    delete bpLayerInterface;
    delete jobSystem;
    delete tempAllocator;
  }

  void PhysicsComponent::OptimizeBroadPhase() {
    if (physicsSystem) {
      physicsSystem->OptimizeBroadPhase();
    }
  }

  SceneNode* PhysicsComponent::CastRay(
    glm::vec3 origin,
    glm::vec3 direction,
    const JPH::BroadPhaseLayerFilter& broadPhaseLayerFilter,
    const JPH::ObjectLayerFilter& objectLayerFilter,
    const JPH::BodyFilter& bodyFilter
  ) {
    JPH::RRayCast ray(
      JPH::RVec3(origin.x, origin.y, origin.z),
      JPH::Vec3(direction.x, direction.y, direction.z)
    );

    JPH::RayCastResult result;
    if (this->physicsSystem->GetNarrowPhaseQuery().CastRay(
      ray,
      result,
      broadPhaseLayerFilter,
      objectLayerFilter,
      bodyFilter
    )) {
      JPH::BodyID id = result.mBodyID;
      uint64_t userData = physicsSystem->GetBodyInterface().GetUserData(id);
      // maybe it would be better to use id as userData if it gets added
      GameObject* object = reinterpret_cast<GameObject*>(userData);
      if (object) {
        return object->GetNode();
      }
    } 
    return nullptr;
  }

  JPH::BodyInterface& PhysicsComponent::GetBodyInterface() {
    return *bodyInterface;
  }

  JPH::PhysicsSystem& PhysicsComponent::GetSystem() {
    return *physicsSystem;
  }
  
  glm::vec3 PhysicsComponent::GetGravity() const {
    const JPH::Vec3 gravity = physicsSystem->GetGravity(); 
    return glm::vec3(
      gravity.GetX(),
      gravity.GetY(),
      gravity.GetZ()
    );
  }

  JPH::GroupFilter* PhysicsComponent::GetLayerGroupFilter() const {
    return this->layerGroupFilter;
  }

  void PhysicsComponent::SetGravity(const glm::vec3 gravity) {
    physicsSystem->SetGravity(Vec3Arg(
      gravity.x,
      gravity.y,
      gravity.z
    ));
  }

  // Sends shapes to the debug rendere
  void PhysicsComponent::OnPostRender() {
    if (drawDebug) {
      auto* debugRenderer = GetScene()->GetComponent<PhysicsDebugRenderer>();
      
      if (debugRenderer) {
        JPH::BodyManager::DrawSettings settings;
        settings.mDrawShape = true;
        settings.mDrawBoundingBox = true;
        settings.mDrawCenterOfMassTransform = true;
        settings.mDrawShapeWireframe = true;

        physicsSystem->DrawBodies(settings, debugRenderer);
        physicsSystem->DrawConstraints(debugRenderer);
      }
    }
  }

  void PhysicsComponent::OnPreUpdate() {
    // TMEPRORARY
    this->accumulator += Time::Delta();
    while (this->accumulator > this->cDeltaTime) { 
      physicsSystem->Update(cDeltaTime, 1, tempAllocator, jobSystem);
      this->accumulator -= this->cDeltaTime;
    }

    // Processing the callback queue 
    std::vector<CollisionData> currentCollisions;
    {
      std::lock_guard<std::mutex> lock(this->collisionMutex);
      currentCollisions = this->collisionQueue;
      this->collisionQueue.clear();
    }
    for (const auto& collision : currentCollisions) {
      GameObject* object1;
      GameObject* object2;
      
      {
        JPH::BodyLockRead lock1(physicsSystem->GetBodyLockInterface(), collision.body1);
        if (lock1.Succeeded()) {
          object1 = reinterpret_cast<GameObject*>(lock1.GetBody().GetUserData());
        }
      }

      {
        JPH::BodyLockRead lock2(physicsSystem->GetBodyLockInterface(), collision.body2);
        if (lock2.Succeeded()) {
          object2 = reinterpret_cast<GameObject*>(lock2.GetBody().GetUserData());
        }
      }

      if (object1 && object2) {
        SceneNode* node1 = object1->GetNode();
        SceneNode* node2 = object2->GetNode();

        for (GameObject* obj : node1->AttachedObjects()) {
          if (auto* receiver = dynamic_cast<IPhysicsCollisionReceiver*>(obj)) {
            receiver->OnCollisionEnter(node2);
          }
        }

        for (GameObject* obj : node2->AttachedObjects()) {
          if (auto* receiver = dynamic_cast<IPhysicsCollisionReceiver*>(obj)) {
            receiver->OnCollisionEnter(node1);
          }
        }
      }
    }

    JPH::BodyIDVector activeBodies;
    physicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);

    for (auto const& bodyId : activeBodies) {
      JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), bodyId);
      if (lock.Succeeded()) {
        const Body& body = lock.GetBody();

        PhysicsObject* object = reinterpret_cast<PhysicsObject*>(body.GetUserData());
        
      if (object) {
        const RVec3& position = body.GetPosition();
        const Quat& rotation = body.GetRotation();

        object->GetTransform().GlobalTransform().Position() = 
          glm::vec3(position.GetX(), position.GetY(), position.GetZ());
        object->GetTransform().GlobalTransform().Rotation() =
          glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
      }
    }
  }

  for (auto& characterObject : this->GetScene()->FindObjectsOfType<PhysicsCharacter>()) {
    characterObject->GetCharacter()->PostSimulation(characterObject->maxSeparationDistance);

    JPH::RVec3 position = characterObject->GetCharacter()->GetPosition();
    JPH::Quat rotation = characterObject->GetCharacter()->GetRotation();

    characterObject->GlobalTransform().Position() =
      glm::vec3(position.GetX(), position.GetY(), position.GetZ());
    characterObject->GlobalTransform().Rotation() =
      glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
  }

  // Queue stuff
}

void PhysicsComponent::DrawImGui() {
	if (ImGui::TreeNode("Physics Debug")) {
    ImGui::Checkbox("Draw collision meshes", &drawDebug);   
	  ImGui::TreePop();
  }
}
