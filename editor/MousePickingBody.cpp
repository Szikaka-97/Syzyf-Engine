#include "MousePickingBody.h"

namespace Editor {
MousePickingBody::MousePickingBody(JPH::BodyCreationSettings settings)
    : Physics::Body(settings) {}

MousePickingBody* MousePickingBody::CreateFromMesh(SceneNode* node,
                                                   const class Mesh* mesh) {
    if (mesh == nullptr) {
        spdlog::error("EditorBody: Tried creating an editor body without "
                      "a valid mesh");
        return nullptr;
    }
    auto settings = Physics::Body::Mesh(mesh, JPH::EMotionType::Kinematic,
                                        Physics::Layers::EDITOR);
    settings.mOverrideMassProperties =
        JPH::EOverrideMassProperties::MassAndInertiaProvided;
    settings.mMassPropertiesOverride.mMass = 1.0f;
    settings.mIsSensor = true;

    MousePickingBody* body = node->AddObject<MousePickingBody>(settings);
    return body;
}

MousePickingBody* MousePickingBody::CreateSphere(SceneNode* node) {
    auto settings = Physics::Body::Sphere(0.3f, JPH::EMotionType::Kinematic,
                                          Physics::Layers::EDITOR);
    settings.mIsSensor = true;

    MousePickingBody* body = node->AddObject<MousePickingBody>(settings);
    return body;
}

void MousePickingBody::SyncToNode() {
    if (GetBodyID().IsInvalid())
        return;

    if (auto* physics = GetScene()->GetComponent<Physics::System>()) {
        glm::vec3 pos = GetNode()->GlobalTransform().Position().Value();

        if (glm::any(glm::isnan(pos)) || glm::any(glm::isinf(pos))) {
            return;
        }

        physics->GetBodyInterface().SetPosition(GetBodyID(),
                                                JPH::RVec3(pos.x, pos.y, pos.z),
                                                JPH::EActivation::DontActivate);
    }
}

} // namespace Editor
