#pragma once

namespace Editor {
class Settings;

class MainMenuBar {
  public:
    void Draw(bool& shouldClose, Settings& settings);
};
} // namespace Editor
