#include "physics/ContactListener.h"
#include "Jolt/Physics/Collision/Shape/SubShapeIDPair.h"
#include "physics/System.h"

namespace Physics {
ContactListener::ContactListener(System* physicsSystem) : physicsSystem(physicsSystem) {}

void ContactListener::OnContactAdded(
  const JPH::Body &inBody1,
  const JPH::Body &inBody2,
  const JPH::ContactManifold &inManifold,
  JPH::ContactSettings &ioSettings
) {
  std::lock_guard<std::mutex> lock(this->physicsSystem->collisionMutex);

  this->physicsSystem->collisionQueue.push_back({
    inBody1.GetID(), inBody2.GetID(),
    CollisionData::State::Enter
  });
}

void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) {
  std::lock_guard<std::mutex> lock(this->physicsSystem->collisionMutex);

  this->physicsSystem->collisionQueue.push_back({
    inSubShapePair.GetBody1ID(),
    inSubShapePair.GetBody2ID(),
    CollisionData::State::Exit
  });
}
    // virtual void OnContactPersisted
}
