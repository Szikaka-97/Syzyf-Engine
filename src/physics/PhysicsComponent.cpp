#include "physics/PhysicsComponent.h"

#include <iostream>
#include <cstdarg>

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
#include <imgui.h>

#include "Jolt/Math/MathTypes.h"
#include "TimeSystem.h"
#include "physics/PhysicsCharacter.h"
#include "physics/PhysicsDebugRenderer.h"
#include "physics/PhysicsObject.h"
#include "Scene.h"

using namespace JPH;
using namespace JPH::literals;

namespace {
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

  // An example contact listener
  class MyContactListener : public ContactListener {
  public:
    // See: ContactListener
    virtual ValidateResult OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override {
      spdlog::info("Contact validate callback");

      // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
      return ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override {
      spdlog::info("A contact was added");
    }

    virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override {
      spdlog::info("A contact was persisted");
    }

    virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override {
      spdlog::info("A contact was removed");
    }
  };

  // An example activation listener
  class MyBodyActivationListener : public BodyActivationListener {
  public:
	  virtual void OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override {
		  spdlog::info("A body got activated");
	  }

	  virtual void OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override {
    spdlog::info("A body went to sleep");
	  }
  };
};

PhysicsComponent::PhysicsComponent(Scene* scene): SceneComponent(scene) {
  tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
  jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

  bpLayerInterface = new BPLayerInterfaceImpl();
  objVsBPFilter = new ObjectVsBroadPhaseLayerFilterImpl();
  objVsObjFilter = new ObjectLayerPairFilterImpl();

  // Add a settings struct?
  const uint cMaxBodies = 1024;
  const uint cNumBodyMutexes = 0;
  const uint cMaxBodyPairs = 1024;
  const uint cMaxContactConstraints = 1024;

  physicsSystem = new PhysicsSystem();
  physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *bpLayerInterface, *objVsBPFilter, *objVsObjFilter);
  bodyInterface = &physicsSystem->GetBodyInterface();
  }

  PhysicsComponent::~PhysicsComponent() {
    delete physicsSystem;
    delete objVsObjFilter;
    delete objVsBPFilter;
    delete bpLayerInterface;
    delete jobSystem;
    delete tempAllocator;
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

  void PhysicsComponent::OnPostUpdate() {
    // TMEPRORARY
    this->accumulator += Time::Delta();
    while (this->accumulator > this->cDeltaTime) { 
      physicsSystem->Update(cDeltaTime, 1, tempAllocator, jobSystem);
      this->accumulator -= this->cDeltaTime;
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
    characterObject->character->PostSimulation(characterObject->maxSeparationDistance);
  }
}

void PhysicsComponent::DrawImGui() {
	if (ImGui::TreeNode("Physics Debug")) {
    ImGui::Checkbox("Draw collision meshes", &drawDebug);   
	  ImGui::TreePop();
  }
}
