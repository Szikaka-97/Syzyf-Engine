#pragma once

class Scene;

namespace Editor {

class Context;

class SceneViewPanel {
  public:
    void Draw(Context& context);

  private:
    void HandleMousePicking(Context& context, float resX, float resY);
};
} // namespace Editor
