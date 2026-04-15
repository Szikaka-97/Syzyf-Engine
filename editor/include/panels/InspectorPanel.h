#pragma once

class SceneNode;

namespace Editor {
class InspectorPanel {
  public:
    void Draw(SceneNode* selectedNode);
};
} // namespace Editor
