#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>

class PhysicsComponent;

class PhysicsContactListener : public JPH::ContactListener {
  private:
    // not sure about this
    PhysicsComponent* physicsComponent;
  public:
    PhysicsContactListener(PhysicsComponent* physicsComponent);

    virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings);

    // virtual void OnContactPersisted
    // virtual void OnContactRemoved
};
