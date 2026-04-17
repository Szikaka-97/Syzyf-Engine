#include "MousePickingBodySystem.h"
#include "InitScene.h"
#include "MousePickingBody.h"
#include "ReflectionProbe.h"

#include <MeshRenderer.h>
#include <Scene.h>
#include <SceneComponent.h>

namespace Editor {
MousePickingBodySystem::MousePickingBodySystem(Scene* scene)
    : SceneComponent(scene) {}

void MousePickingBodySystem::OnPreUpdate() {
    UpdateBodies(GetScene()->GetRootNode());
}

void MousePickingBodySystem::UpdateBodies(SceneNode* node) {
    if (!node)
        return;

    MousePickingBody* body = node->GetObject<MousePickingBody>();

    if (!body && !node->GetObject<EditorCameraTag>()) {
        if (auto* mr = node->GetObject<MeshRenderer>()) {
            body = MousePickingBody::CreateFromMesh(node, mr->GetMesh());
        } else if (node->GetObject<Light>() ||
                   node->GetObject<ReflectionProbe>()) {
            // enable this for other objects as well
            body = MousePickingBody::CreateSphere(node);
        }
    }

    if (body) {
        body->SyncToNode();
    }

    for (auto* child : node->GetChildren()) {
        UpdateBodies(child);
    }
}
} // namespace Editor
