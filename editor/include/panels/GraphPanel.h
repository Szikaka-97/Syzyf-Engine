#pragma once

#include <imgui.h>
#include <nlohmann/json_fwd.hpp>

class Scene;
class SceneNode;

namespace Editor {
class Context;

class GraphPanel {
  public:
    void Draw(Context& context);

  private:
    void DrawGraphNode(Context& context, SceneNode& node);
    void DrawContextMenu(Context& context);

    void HandleSavePrefab(nlohmann::json j);
};
} // namespace Editor
