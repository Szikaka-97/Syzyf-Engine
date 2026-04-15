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
        auto settings = Physics::Body::Mesh(mesh, JPH::EMotionType::Static,
                                            Physics::Layers::EDITOR);
        settings.mIsSensor = true;

        EditorBody* body = node->AddObject<EditorBody>(settings);
        return body;
    }

    static EditorBody* CreateSphere(SceneNode* node) {
        auto settings = Physics::Body::Sphere(0.3f, JPH::EMotionType::Static,
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

            physics->GetBodyInterface().SetPosition(
                GetBodyID(), JPH::RVec3(pos.x, pos.y, pos.z),
                JPH::EActivation::DontActivate);
        }
    }
};
