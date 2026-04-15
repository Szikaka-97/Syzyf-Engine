#pragma once

#include <imgui.h>

class Scene;
class SceneNode;

namespace Editor {
class GraphPanel {
  public:
    void Draw(Scene* selectedScene, SceneNode* selectedNode);

  private:
    void DrawGraphNode(SceneNode& node, SceneNode* selectedNode);
};
} // namespace Editor
