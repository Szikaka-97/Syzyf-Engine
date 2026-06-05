#include <Jolt/Jolt.h>

#include "Jolt/Physics/Collision/BroadPhase/BroadPhase.h"
#include "Jolt/Physics/Collision/TransformedShape.h"
#include "physics/Body.h"
#include "physics/CharacterController.h"
#include "physics/ContactListener.h"
#include "physics/DebugRenderer.h"
#include "physics/ICollisionReceiver.h"
#include "physics/System.h"

#include "GameObject.h"
#include "Scene.h"
#include "TimeSystem.h"
#include "physics/ICollisionReceiver.h"
#include "physics/VirtualCharacterController.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Math/MathTypes.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>
#include <imgui.h>

using namespace JPH::literals;

namespace Physics {
class GroupFilterLayerMask : public JPH::GroupFilter {
  public:
    virtual bool
    CanCollide(const JPH::CollisionGroup& inGroup1,
               const JPH::CollisionGroup& inGroup2) const override {
        return ((inGroup1.GetGroupID() & inGroup2.GetSubGroupID()) != 0) ||
               ((inGroup2.GetGroupID() & inGroup1.GetSubGroupID()) != 0);
    }
};

// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1,
                               JPH::ObjectLayer inObject2) const override {
        if (inObject1 == Layers::EDITOR || inObject2 == Layers::EDITOR)
            return false;

        switch (inObject1) {
        case Layers::NON_MOVING:
            return inObject2 ==
                   Layers::MOVING; // Non moving only collides with moving
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
        mObjectToBroadPhase[Layers::EDITOR] = BroadPhaseLayers::EDITOR;
    }

    virtual unsigned int GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer
    GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char*
    GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
            return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
            return "MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::EDITOR:
            return "EDITOR";
        default:
            JPH_ASSERT(false);
            return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

  private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl
    : public JPH::ObjectVsBroadPhaseLayerFilter {
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
                               JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        case Layers::EDITOR:
            return false;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

System::System(Scene* scene, const SystemSettings& settings)
    : SceneComponent(scene) {
    this->characterControllerSystem = scene->AddComponent<CharacterControllerSystem>();
    this->virtualCharacterControllerSystem = scene->AddComponent<VirtualCharacterControllerSystem>();
    this->bodySystem = scene->AddComponent<BodySystem>();

    layerGroupFilter = new GroupFilterLayerMask();

    tempAllocator = new JPH::TempAllocatorImpl(settings.tempAllocatorSize);
    jobSystem = new JPH::JobSystemThreadPool(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        JPH::thread::hardware_concurrency() - 1);
    bpLayerInterface = new BPLayerInterfaceImpl();
    objVsBPFilter = new ObjectVsBroadPhaseLayerFilterImpl();
    objVsObjFilter = new ObjectLayerPairFilterImpl();

    physicsSystem = new JPH::PhysicsSystem();
    physicsSystem->Init(settings.maxBodies, settings.numBodyMutexes,
                        settings.maxBodyPairs, settings.maxContactConstraints,
                        *bpLayerInterface, *objVsBPFilter, *objVsObjFilter);
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

RayCastPayload
System::CastRay(glm::vec3 origin, glm::vec3 direction,
                const JPH::BroadPhaseLayerFilter& broadPhaseLayerFilter,
                const JPH::ObjectLayerFilter& objectLayerFilter,
                const JPH::BodyFilter& bodyFilter) {
    RayCastPayload payload;

    JPH::RRayCast ray(JPH::RVec3(origin.x, origin.y, origin.z),
                      JPH::Vec3(direction.x, direction.y, direction.z));

    JPH::RayCastResult result;
    if (this->physicsSystem->GetNarrowPhaseQuery().CastRay(
            ray, result, broadPhaseLayerFilter, objectLayerFilter,
            bodyFilter)) {
        payload.hasHit = true;
        payload.fraction = result.mFraction;

        JPH::RVec3 joltPos = ray.GetPointOnRay(result.mFraction);
        payload.position =
            glm::vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());

        JPH::BodyID id = result.mBodyID;
        uint64_t userData = physicsSystem->GetBodyInterface().GetUserData(id);
        GameObject* object = reinterpret_cast<GameObject*>(userData);
        if (object) {
            payload.node = object->GetNode();
        }
    }
    return payload;
}

std::vector<SceneNode*>
System::CastShape(glm::vec3 origin, glm::vec3 direction,
                  const JPH::ShapeRefC& shape,
                  const JPH::BroadPhaseLayerFilter& broadPhaseLayerFilter,
                  const JPH::ObjectLayerFilter& objectLayerFilter,
                  const JPH::BodyFilter& bodyFilter) {
    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
        shape, JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z)),
        JPH::Vec3(direction.x, direction.y, direction.z));

    JPH::ShapeCastSettings settings;

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;

    this->physicsSystem->GetNarrowPhaseQuery().CastShape(
        shapeCast, settings, shapeCast.mCenterOfMassStart.GetTranslation(),
        collector, broadPhaseLayerFilter, objectLayerFilter, bodyFilter);

    std::vector<SceneNode*> result;
    result.reserve(collector.mHits.size());

    for (auto body : collector.mHits) {
        result.push_back(
            reinterpret_cast<GameObject*>(
                this->physicsSystem->GetBodyInterface().GetUserData(
                    body.mBodyID2))
                ->GetNode());
    }

    // Could add an option to sort the vector
    return result;
}

JPH::BodyInterface& System::GetBodyInterface() { return *bodyInterface; }

JPH::PhysicsSystem* System::GetJoltSystem() { return physicsSystem; }

JPH::TempAllocatorImpl& System::GetTempAllocator() {
    return *this->tempAllocator;
}

glm::vec3 System::GetGravity() const {
    const JPH::Vec3 gravity = physicsSystem->GetGravity();
    return glm::vec3(gravity.GetX(), gravity.GetY(), gravity.GetZ());
}

JPH::GroupFilter* System::GetLayerGroupFilter() const {
    return this->layerGroupFilter;
}

void System::SetGravity(const glm::vec3 gravity) {
    physicsSystem->SetGravity(JPH::Vec3Arg(gravity.x, gravity.y, gravity.z));
}

void System::OnPreUpdate() {
    // TMEPRORARY
    if (this->firstFrame) {
        this->firstFrame = false;
        this->accumulator = 0.0f;
        return;
    }

    this->accumulator += Time::UnscaledDelta();

    float maxAccumulator = this->cDeltaTime * 8.0f;
    if (this->accumulator > maxAccumulator) {
        this->accumulator = maxAccumulator;
    }

    float scaledStep = this->cDeltaTime * Time::GetTimeScale();
    bool physicsStepped = false;

    while (this->accumulator >= this->cDeltaTime) {
        if (scaledStep > 0.0f) {
            physicsSystem->Update(scaledStep, 1, tempAllocator, jobSystem);
            physicsStepped = true;
        }

        this->accumulator -= this->cDeltaTime;
    }

    if (physicsStepped) {
        // Processing the callback queue
        std::vector<CollisionData> currentCollisions;
        {
            std::lock_guard<std::mutex> lock(this->collisionMutex);
            currentCollisions = this->collisionQueue;
            this->collisionQueue.clear();
        }
        for (const auto& collision : currentCollisions) {

            GameObject* object1 = nullptr;
            GameObject* object2 = nullptr;

            {
                JPH::BodyLockRead lock1(physicsSystem->GetBodyLockInterface(),
                                        collision.body1);
                if (lock1.Succeeded()) {
                    object1 = reinterpret_cast<GameObject*>(
                        lock1.GetBody().GetUserData());
                }
            }

            {
                JPH::BodyLockRead lock2(physicsSystem->GetBodyLockInterface(),
                                        collision.body2);
                if (lock2.Succeeded()) {
                    object2 = reinterpret_cast<GameObject*>(
                        lock2.GetBody().GetUserData());
                }
            }

            if (object1 && object2) {
                SceneNode* node1 = object1->GetNode();
                SceneNode* node2 = object2->GetNode();

                for (GameObject* obj : node1->AttachedObjects()) {
                    if (auto* receiver =
                            dynamic_cast<ICollisionReceiver*>(obj)) {
                        if (collision.state == CollisionData::State::Enter)
                            receiver->OnCollisionEnter(node2);
                        else
                            receiver->OnCollisionExit(node2);
                    }
                }

                for (GameObject* obj : node2->AttachedObjects()) {
                    if (auto* receiver =
                            dynamic_cast<ICollisionReceiver*>(obj)) {
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
            JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(),
                                   bodyId);
            if (lock.Succeeded()) {
                const JPH::Body& body = lock.GetBody();

                GameObject* object = reinterpret_cast<GameObject*>(body.GetUserData());

                if (object && body.GetObjectLayer() != Layers::EDITOR) {
                    const JPH::RVec3& position = body.GetPosition();
                    const JPH::Quat& rotation = body.GetRotation();

                    object->GetTransform().GlobalTransform().Position() =
                        glm::vec3(position.GetX(), position.GetY(),
                                  position.GetZ());
                    object->GetTransform().GlobalTransform().Rotation() =
                        glm::quat(rotation.GetW(), rotation.GetX(),
                                  rotation.GetY(), rotation.GetZ());
                }
            }
        }

    {
        for (auto& characterObject : this->characterControllerSystem->IterateObjects()) {
            characterObject->GetCharacter()->PostSimulation(
                characterObject->maxSeparationDistance);

            JPH::RVec3 position =
                characterObject->GetCharacter()->GetPosition();
            JPH::Quat rotation = characterObject->GetCharacter()->GetRotation();

            characterObject->GlobalTransform().Position() =
                glm::vec3(position.GetX(), position.GetY(), position.GetZ());
            characterObject->GlobalTransform().Rotation() =
                glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(),
                          rotation.GetZ());
        }
    }
    }
    // Queue stuff
}

void Physics::System::DrawPhysicsDebug(DebugRenderer* debugRenderer) {
    if (drawDebug && debugRenderer) {
        JPH::BodyManager::DrawSettings settings;
        settings.mDrawShape = true;
        settings.mDrawBoundingBox = true;
        settings.mDrawCenterOfMassTransform = true;
        settings.mDrawShapeWireframe = true;

        physicsSystem->DrawBodies(settings, debugRenderer,
                                  &debugRenderer->filter);
        physicsSystem->DrawConstraints(debugRenderer);

        for (auto& characterObject :
             this->virtualCharacterControllerSystem->IterateObjects()) {
            auto character = characterObject->GetCharacter();

            character->GetShape()->Draw(
                debugRenderer, character->GetCenterOfMassTransform(),
                JPH::Vec3::sReplicate(1.0f), JPH::Color::sOrange, false, true);
        }
    }
}

void System::DrawImGui() {
    if (ImGui::TreeNode("Physics Debug")) {
        ImGui::Checkbox("Draw collision meshes", &drawDebug);
        ImGui::TreePop();
    }
}
}; // namespace Physics
