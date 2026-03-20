#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>

namespace Physics {

class System;

class ContactListener : public JPH::ContactListener {
  private:
    // not sure about this
    System* physicsSystem;
  public:
    ContactListener(System* physicsSystem);

    virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings);

    // virtual void OnContactPersisted
    // virtual void OnContactRemoved
};
}
