#pragma once

namespace Editor {
class Settings;
class Context;

class MainMenuBar {
  public:
    void Draw(Context& context, bool& shouldClose, Settings& settings);
};
} // namespace Editor
