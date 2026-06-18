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
    char searchBuffer[256] = "";

    void DrawGraphNode(Context& context, SceneNode& node,
                       const std::string& searchString);
    void DrawContextMenu(Context& context);

    bool NodeMatchesSearch(SceneNode& node, const std::string& searchString);
};
} // namespace Editor
