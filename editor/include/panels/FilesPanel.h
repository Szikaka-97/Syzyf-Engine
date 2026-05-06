#pragma once

#include <Texture.h>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace Editor {
class FilesPanel {
  public:
    void Draw();

  private:
    struct ThumbnailResult {
        std::string path;
        unsigned char* pixels;
        int width, height;
    };

    const float PADDING = 16.0f;
    const float THUMBNAIL_SIZE = 64.0f;
    const float CELL_SIZE = THUMBNAIL_SIZE + PADDING;

    // Preview Panel stuff
    bool showPreviewPopup = false;
    std::string previewFilePath;
    std::string previewTextContent;
    std::unique_ptr<Texture2D> previewTexture;

    std::filesystem::path currentDirectory = "./res/";
    char searchBuffer[256] = "";

    std::unique_ptr<Texture2D> folderIcon;
    std::unique_ptr<Texture2D> fileIcon;
    std::unique_ptr<Texture2D> modelIcon;
    std::unique_ptr<Texture2D> audioIcon;
    std::unique_ptr<Texture2D> codeIcon;

    std::unordered_map<std::string, std::unique_ptr<Texture2D>> thumbnails;
    std::vector<ThumbnailResult> pendingResults;
    std::unordered_set<std::string> loadingPaths;
    std::mutex resultsMutex;

    void DrawPreviewPopup();

    Texture2D* GetOrCreateThumbnail(const std::filesystem::path& path);
    void RequestThumbnail(const std::filesystem::path& path);
    void ProcessPendingThumbnails();

    void GetIcons();
};
} // namespace Editor
