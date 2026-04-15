#pragma once

#include <imgui.h>

class Scene;
class SceneNode;

namespace Editor {
class Context;

class GraphPanel {
  public:
    void Draw(Context& context);

  private:
    void DrawGraphNode(Context& context, SceneNode& node);
};
} // namespace Editor
