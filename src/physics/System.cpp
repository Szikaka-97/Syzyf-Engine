#include "physics/System.h"
#include "Jolt/Physics/Collision/TransformedShape.h"
#include "physics/CharacterController.h"
#include "physics/ICollisionReceiver.h"
#include "physics/DebugRenderer.h"
#include "physics/Body.h"
#include "physics/ContactListener.h"

#include "GameObject.h"
#include "TimeSystem.h"
#include "Scene.h"
#include "physics/ICollisionReceiver.h"

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
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <imgui.h>

using namespace JPH::literals;

namespace Physics {
  class GroupFilterLayerMask : public JPH::GroupFilter {
  public:
    virtual bool CanCollide(const JPH::CollisionGroup& inGroup1, const JPH::CollisionGroup& inGroup2) const override {
      return ((inGroup1.GetGroupID() & inGroup2.GetSubGroupID()) != 0) ||
             ((inGroup2.GetGroupID() & inGroup1.GetSubGroupID()) != 0);
    }
  };
  
  // Class that determines if two object layers can collide
  class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
      switch (inObject1) {
      case Layers::NON_MOVING:
        return inObject2 == Layers::MOVING; // Non moving only collides with moving
      case Layers::MOVING:
  return true; // Moving collides with everything
      default:
        JPH_ASSERT(false);
        return false;
      }
    }
  };

  // BroadPhaseLayerInterface implementation
  // This defines a mapping between object and broadphase layers.
  class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
  public:
    BPLayerInterfaceImpl() {
      // Create a mapping table from object to broad phase layer
      mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
      mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual unsigned int GetNumBroadPhaseLayers() const override {
      return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override{
      JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
      return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override{
      switch ((JPH::BroadPhaseLayer::Type)inLayer) {
      case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
      case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
      default: JPH_ASSERT(false); return "INVALID";
      }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

  private:
    JPH::BroadPhaseLayer	mObjectToBroadPhase[Layers::NUM_LAYERS];
  };

  // Class that determines if an object layer can collide with a broadphase layer
  class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
      switch (inLayer1) {
      case Layers::NON_MOVING:
        return inLayer2 == BroadPhaseLayers::MOVING;
      case Layers::MOVING:
        return true;
      default:
        JPH_ASSERT(false);
        return false;
      }
    }
  };

  System::System(Scene* scene, const SystemSettings& settings): SceneComponent(scene) {
    layerGroupFilter = new GroupFilterLayerMask();
    
    tempAllocator = new JPH::TempAllocatorImpl(settings.tempAllocatorSize);
    jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);
    bpLayerInterface = new BPLayerInterfaceImpl();
    objVsBPFilter = new ObjectVsBroadPhaseLayerFilterImpl();
    objVsObjFilter = new ObjectLayerPairFilterImpl();

    physicsSystem = new JPH::PhysicsSystem();
    physicsSystem->Init(settings.maxBodies, settings.numBodyMutexes, settings.maxBodyPairs, settings.maxContactConstraints, *bpLayerInterface, *objVsBPFilter, *objVsObjFilter);
    bodyInterface = &physicsSystem->GetBodyInterface();

    contactListener = new ContactListener(this);
    physicsSystem->SetContactListener(contactListener);
    physicsSystem->SetBodyActivationListener(bodyActivationListener);
  }

  System::~System() {
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

  void System::OptimizeBroadPhase() {
    if (physicsSystem) {
      physicsSystem->OptimizeBroadPhase();
    }
  }

  SceneNode* System::CastRay(
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

  std::vector<SceneNode*> System::CastShape(
    glm::vec3 origin,
    glm::vec3 direction,
    const JPH::ShapeRefC& shape,
    const JPH::BroadPhaseLayerFilter& broadPhaseLayerFilter,
    const JPH::ObjectLayerFilter& objectLayerFilter,
    const JPH::BodyFilter& bodyFilter
  ) {
    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
      shape,
      JPH::Vec3::sReplicate(1.0f),
      JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z)),
      JPH::Vec3(direction.x, direction.y, direction.z)
    );

    JPH::ShapeCastSettings settings;

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;

    this->physicsSystem->GetNarrowPhaseQuery().CastShape(
      shapeCast,
      settings,
      shapeCast.mCenterOfMassStart.GetTranslation(),
      collector,
      broadPhaseLayerFilter,
      objectLayerFilter,
      bodyFilter
    );

    std::vector<SceneNode*> result;
    result.reserve(collector.mHits.size());

    for (auto body : collector.mHits) {
      result.push_back(
        reinterpret_cast<GameObject*>(this->physicsSystem->GetBodyInterface().GetUserData(body.mBodyID2))->GetNode()
      );
    }

    // Could add an option to sort the vector
    return result;
  }

  JPH::BodyInterface& System::GetBodyInterface() {
    return *bodyInterface;
  }

  JPH::PhysicsSystem& System::GetSystem() {
    return *physicsSystem;
  }

  JPH::TempAllocatorImpl& System::GetTempAllocator() {
    return *this->tempAllocator;
  }
  
  glm::vec3 System::GetGravity() const {
    const JPH::Vec3 gravity = physicsSystem->GetGravity(); 
    return glm::vec3(
      gravity.GetX(),
      gravity.GetY(),
      gravity.GetZ()
    );
  }

  JPH::GroupFilter* System::GetLayerGroupFilter() const {
    return this->layerGroupFilter;
  }

  void System::SetGravity(const glm::vec3 gravity) {
    physicsSystem->SetGravity(JPH::Vec3Arg(
      gravity.x,
      gravity.y,
      gravity.z
    ));
  }

  // Sends shapes to the debug rendere
  void System::OnPostRender() {
    if (drawDebug) {
      auto* debugRenderer = GetScene()->GetComponent<DebugRenderer>();
      
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

  void System::OnPreUpdate() {
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
          if (auto* receiver = dynamic_cast<ICollisionReceiver*>(obj)) {
            if (collision.state == CollisionData::State::Enter)
              receiver->OnCollisionEnter(node2);
            else
              receiver->OnCollisionExit(node2);
          }
        }

        for (GameObject* obj : node2->AttachedObjects()) {
          if (auto* receiver = dynamic_cast<ICollisionReceiver*>(obj)) {
            if (collision.state == CollisionData::State::Enter)
              receiver->OnCollisionEnter(node1);
            else
              receiver->OnCollisionExit(node1);
          }
        }
      }
    }

    JPH::BodyIDVector activeBodies;
    physicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);

    for (auto const& bodyId : activeBodies) {
      JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), bodyId);
      if (lock.Succeeded()) {
        const JPH::Body& body = lock.GetBody();

        Body* object = reinterpret_cast<Body*>(body.GetUserData());
        
      if (object) {
        const JPH::RVec3& position = body.GetPosition();
        const JPH::Quat& rotation = body.GetRotation();

        object->GetTransform().GlobalTransform().Position() = 
          glm::vec3(position.GetX(), position.GetY(), position.GetZ());
        object->GetTransform().GlobalTransform().Rotation() =
          glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
      }
    }
  }

  for (auto& characterObject : this->GetScene()->FindObjectsOfType<CharacterController>()) {
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

void System::DrawImGui() {
	if (ImGui::TreeNode("Physics Debug")) {
    ImGui::Checkbox("Draw collision meshes", &drawDebug);   
	  ImGui::TreePop();
  }
}
};
