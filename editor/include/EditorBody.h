#pragma once

#include "Mesh.h"
#include <physics/Body.h>
#include <physics/System.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

class EditorBody : public Physics::Body {
  public:
    EditorBody(JPH::BodyCreationSettings settings) : Physics::Body(settings) {}

    static EditorBody* CreateFromMesh(SceneNode* node, const class Mesh* mesh) {
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

        EditorBody* body = node->AddObject<EditorBody>(settings);
        return body;
    }

    static EditorBody* CreateSphere(SceneNode* node) {
        auto settings = Physics::Body::Sphere(0.3f, JPH::EMotionType::Kinematic,
                                              Physics::Layers::EDITOR);
        settings.mIsSensor = true;

        EditorBody* body = node->AddObject<EditorBody>(settings);
        return body;
    }

    void SyncToNode() {
        if (GetBodyID().IsInvalid())
            return;

        if (auto* physics = GetScene()->GetComponent<Physics::System>()) {
            glm::vec3 pos = GetNode()->GlobalTransform().Position().Value();

            if (glm::any(glm::isnan(pos)) || glm::any(glm::isinf(pos))) {
                return;
            }

            physics->GetBodyInterface().SetPosition(
                GetBodyID(), JPH::RVec3(pos.x, pos.y, pos.z),
                JPH::EActivation::DontActivate);
        }
    }
};
