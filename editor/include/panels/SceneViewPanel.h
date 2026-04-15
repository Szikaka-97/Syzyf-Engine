#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
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

    EditorLayerFilter filter;

  public:
    void Draw(Context& context);

  private:
    void HandleMousePicking(Context& context, float resX, float resY);
};
} // namespace Editor
