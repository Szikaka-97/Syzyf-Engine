#pragma once

#include "KeyboardControls.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <glm/glm.hpp>
#include <physics/System.h>

class Scene;

namespace Editor {

class Context;

class SceneViewPanel {
  private:
    class EditorLayerFilter : public JPH::ObjectLayerFilter {
      public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override {
            return inLayer == Physics::Layers::EDITOR;
        }
    };

    bool wasViewGuizmoUsed = false;
    glm::mat4 initialGlobalTransform;
    glm::mat4 initialLocalTransform;

    EditorLayerFilter filter;

    KeyboardControls keyboardControls;

  public:
    void Draw(Context& context);

  private:
    void UpdateAndRenderScene(Context& context);

    void HandleMousePicking(Context& context, float resX, float resY);
    void HandleDrop(Context& context);

    void DrawMenuBar(Context& context);
};
} // namespace Editor
