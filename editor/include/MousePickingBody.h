#pragma once

#include "Mesh.h"
#include <physics/Body.h>
#include <physics/System.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

namespace Editor {

class MousePickingBody : public Physics::Body {
  public:
    MousePickingBody(JPH::BodyCreationSettings settings);

    static MousePickingBody* CreateFromMesh(SceneNode* node,
                                            const class Mesh* mesh);

    static MousePickingBody* CreateSphere(SceneNode* node);

    void SyncToNode();
};
} // namespace Editor
