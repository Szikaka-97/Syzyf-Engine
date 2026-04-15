#include "EditorBody.h"
#include "InitScene.h"

#include <MeshRenderer.h>
#include <Scene.h>
#include <SceneComponent.h>

class EditorBodySystem : public SceneComponent {
  public:
    EditorBodySystem(Scene* scene) : SceneComponent(scene) {}

    void OnPreUpdate() override { UpdateBodies(GetScene()->GetRootNode()); }

  private:
    void UpdateBodies(SceneNode* node) {
        if (!node)
            return;

        EditorBody* body = node->GetObject<EditorBody>();

        if (!body && !node->GetObject<EditorCameraTag>()) {
            if (auto* mr = node->GetObject<MeshRenderer>()) {
                body = EditorBody::CreateFromMesh(node, mr->GetMesh());
            } else if (node->GetObject<Light>()) {
                // enable this for other objects as well
                body = EditorBody::CreateSphere(node);
            }
        }

        if (body) {
            body->SyncToNode();
        }

        for (auto* child : node->GetChildren()) {
            UpdateBodies(child);
        }
    }
};
