#include "physics/ContactListener.h"
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
    inBody1.GetID(), inBody2.GetID()
  });
}
    // virtual void OnContactPersisted
    // virtual void OnContactRemoved
}
