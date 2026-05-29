#pragma once

namespace Editor {
class Context;

// Rename to something else later
class SystemsDebugPanel {
  private:
    char componentSearchBuffer[256] = "";
    bool focusComponentSearch = false;
  public:
    void Draw(Context& context);
};
} // namespace Editor
