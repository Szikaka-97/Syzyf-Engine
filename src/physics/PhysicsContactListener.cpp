#include "physics/PhysicsContactListener.h"

#include "physics/PhysicsComponent.h"

PhysicsContactListener::PhysicsContactListener(PhysicsComponent* physicsComponent) : physicsComponent(physicsComponent) {}

void PhysicsContactListener::OnContactAdded(
  const JPH::Body &inBody1,
  const JPH::Body &inBody2,
  const JPH::ContactManifold &inManifold,
  JPH::ContactSettings &ioSettings
) {
  std::lock_guard<std::mutex> lock(this->physicsComponent->collisionMutex);

  this->physicsComponent->collisionQueue.push_back({
    inBody1.GetID(), inBody2.GetID()
  });
}
    // virtual void OnContactPersisted
    // virtual void OnContactRemoved
