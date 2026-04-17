#pragma once

class SceneNode;

namespace Editor {
class Context;

class InspectorPanel {
  public:
    void Draw(Context& context);

  private:
    bool showAddComponentWindow = false;
};
} // namespace Editor
