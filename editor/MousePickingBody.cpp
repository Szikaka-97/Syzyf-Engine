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
    auto settings = Physics::Body::Mesh(mesh, JPH::EMotionType::Static,
                                        Physics::Layers::EDITOR);
    settings.mOverrideMassProperties =
        JPH::EOverrideMassProperties::MassAndInertiaProvided;
    settings.mIsSensor = true;

    MousePickingBody* body = node->AddObject<MousePickingBody>(settings);
    return body;
}

MousePickingBody* MousePickingBody::CreateSphere(SceneNode* node) {
    auto settings = Physics::Body::Sphere(0.3f, JPH::EMotionType::Static,
                                          Physics::Layers::EDITOR);
    settings.mIsSensor = true;

    MousePickingBody* body = node->AddObject<MousePickingBody>(settings);
    return body;
}

void MousePickingBody::SyncToNode() {
    if (GetBodyID().IsInvalid())
        return;

    if (auto* physics = GetScene()->GetComponent<Physics::System>()) {
        glm::vec3 position = GetNode()->GlobalTransform().Position().Value();
        glm::quat rotation = GetNode()->GlobalTransform().Rotation().Value();
        glm::vec3 scale = GetNode()->GlobalTransform().Scale().Value();

        const float MAX_VALID_POSITION = 1000000.0f;

        if (glm::any(glm::isnan(position)) || glm::any(glm::isinf(position)) ||
            glm::any(glm::greaterThan(glm::abs(position),
                                      glm::vec3(MAX_VALID_POSITION)))) {
            return;
        }

        physics->GetBodyInterface().SetPositionAndRotation(
            GetBodyID(), JPH::RVec3(position.x, position.y, position.z),
            JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
            JPH::EActivation::DontActivate);
    }
}

} // namespace Editor
