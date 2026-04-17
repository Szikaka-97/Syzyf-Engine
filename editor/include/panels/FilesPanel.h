#pragma once

#include <filesystem>

namespace Editor {
class FilesPanel {
  public:
    void Draw();

  private:
    const float PADDING = 16.0f;
    const float THUMBNAIL_SIZE = 64.0f;
    const float CELL_SIZE = THUMBNAIL_SIZE + PADDING;

    std::filesystem::path currentDirectory = "./res/";
};
} // namespace Editor
