#include "physics/Water.h"
#include "TimeSystem.h"
#include "physics/Body.h"
#include "physics/System.h"

namespace Physics {

void Water::OnCollisionEnter(SceneNode* otherNode) {
  submergedNodes.insert(otherNode);
}

void Water::OnCollisionExit(SceneNode* otherNode) {
  submergedNodes.erase(otherNode);
}

void Water::Update() {
  if (submergedNodes.empty()) return;

  auto* physicsSystem = this->GetScene()->GetComponent<Physics::System>();
  JPH::BodyInterface& bodyInterface = physicsSystem->GetSystem().GetBodyInterface();

  if (!this->GetObject<Physics::Body>()->IsSensor()) {
    this->GetObject<Physics::Body>()->SetIsSensor(true);
  }

  JPH::RVec3 surfacePosition(this->GlobalTransform().Position().x, this->GlobalTransform().Position().y + 1.0f, this->GlobalTransform().Position().z);
  JPH::Vec3 surfaceNormal(0, 1, 0);

  for (auto it = submergedNodes.begin(); it != submergedNodes.end(); ) {
    SceneNode* node = *it;
    Physics::Body* body = node->GetObject<Physics::Body>();

    if (!body || !body->IsActive() || !body->IsEnabled()) {
      it = submergedNodes.erase(it);
      continue;
    }

    float buoyancyMultiplier = 1.2f; // move to material

    bodyInterface.ApplyBuoyancyImpulse(
      body->GetBodyID(),
      surfacePosition,
      surfaceNormal,
      buoyancyMultiplier,
      0.5f,
      0.5f,
      JPH::Vec3::sZero(),
      JPH::Vec3(
        physicsSystem->GetGravity().x,
        physicsSystem->GetGravity().y,
        physicsSystem->GetGravity().z
      ),
      Time::Delta()
    );

    ++it;
  }
}

}
