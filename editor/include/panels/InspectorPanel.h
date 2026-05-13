#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class SceneNode;

namespace Editor {
class Context;

class InspectorPanel {
  public:
    void Draw(Context& context);

  private:
    glm::vec3 initialPosition;
    glm::quat initialRotation;
    glm::vec3 initialScale;

    char componentSearchBuffer[256] = "";
    bool focusComponentSearch = false;
};
} // namespace Editor
