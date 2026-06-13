#pragma once

#include <MeshRenderer.h>
#include <Scene.h>
#include <SceneComponent.h>

namespace Editor {
class MousePickingBodySystem : public SceneComponent {
  public:
    MousePickingBodySystem(Scene* scene);
    void OnPreUpdate() override;

  private:
    void UpdateBodies(SceneNode* node);
};
} // namespace Editor
