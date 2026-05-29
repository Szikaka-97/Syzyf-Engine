#pragma once

namespace Editor {
struct Context;

// Rename to something else later
class SystemsDebugPanel {
  private:
    char componentSearchBuffer[256] = "";
    bool focusComponentSearch = false;
  public:
    void Draw(Context& context);
};
} // namespace Editor
