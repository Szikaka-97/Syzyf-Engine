#include "panels/FilesPanel.h"

#include <Texture.h>
#include <cctype>
#include <filesystem>
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_resize2.h>

namespace fs = std::filesystem;

namespace Editor {
void FilesPanel::Draw() {
    this->ProcessPendingThumbnails();

    this->GetIcons();

    ImGui::Begin("Files");

    if (this->currentDirectory != "./res/" &&
        this->currentDirectory != "./res") {
        if (ImGui::Button("Back")) {
            this->currentDirectory = this->currentDirectory.parent_path();
        }
        ImGui::Separator();
    }

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / this->CELL_SIZE);
    if (columnCount < 1) {
        columnCount = 1;
    }

    if (ImGui::BeginTable("FileBrowserGrid", columnCount)) {
        if (fs::exists(this->currentDirectory)) {
            for (auto& directoryEntry :
                 fs::directory_iterator(this->currentDirectory)) {
                ImGui::TableNextColumn();

                const auto& path = directoryEntry.path();
                std::string filenameString = path.filename().string();

                ImGui::PushID(filenameString.c_str());

                Texture2D* iconTexture = this->fileIcon.get();
                if (directoryEntry.is_directory()) {
                    iconTexture = this->folderIcon.get();
                } else {
                    std::string extension = path.extension().string();
                    std::transform(extension.begin(), extension.end(),
                                   extension.begin(), ::tolower);

                    if (extension == ".mp3" || extension == ".wav")
                        iconTexture = this->audioIcon.get();
                    if (extension == ".vert" || extension == ".frag" ||
                        extension == ".comp" || extension == ".geom" ||
                        extension == ".tess_eval" || extension == ".tess_ctrl")
                        iconTexture = this->codeIcon.get();
                    if (extension == ".obj" || extension == ".glb" ||
                        extension == ".gltf" || extension == ".fbx" ||
                        extension == ".blend")
                        iconTexture = this->modelIcon.get();
                }

                if (!directoryEntry.is_directory()) {
                    Texture2D* thumbnail = GetOrCreateThumbnail(path);
                    if (thumbnail) {
                        iconTexture = thumbnail;
                    }
                }

                GLuint textureID = iconTexture ? iconTexture->GetHandle() : 0;

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                bool clicked = ImGui::ImageButton(
                    filenameString.c_str(), (ImTextureID)(intptr_t)textureID,
                    ImVec2(this->THUMBNAIL_SIZE, this->THUMBNAIL_SIZE));

                ImGui::PopStyleColor();

                if (clicked) {
                    if (directoryEntry.is_directory()) {
                        this->currentDirectory /= path.filename();
                        ImGui::PopID();
                        break;
                    }
                }

                if (!directoryEntry.is_directory()) {
                    std::string fullPath = path.string();

                    if (ImGui::BeginDragDropSource(
                            ImGuiDragDropFlags_SourceAllowNullID)) {
                        ImGui::SetDragDropPayload("DND_FILE_PATH",
                                                  fullPath.c_str(),
                                                  fullPath.size() + 1);

                        ImGui::Text("Drop %s", filenameString.c_str());

                        ImGui::EndDragDropSource();
                    }
                }

                ImGui::TextWrapped("%s", filenameString.c_str());

                ImGui::PopID();
            }
        } else {
            ImGui::Text("Directory %s not found",
                        this->currentDirectory.string().c_str());
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

Texture2D* FilesPanel::GetOrCreateThumbnail(const fs::path& path) {
    std::string pathString = path.string();

    auto it = thumbnails.find(pathString);
    if (it != thumbnails.end()) {
        return it->second.get();
    }

    if (loadingPaths.contains(pathString)) {
        return nullptr;
    }

    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   ::tolower);

    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
        extension == ".bmp" || extension == ".hdr") {
        RequestThumbnail(path);
    }

    return nullptr;
}

void FilesPanel::RequestThumbnail(const std::filesystem::path& path) {
    std::string pathString = path.string();
    loadingPaths.insert(pathString);

    std::thread([this, pathString]() {
        int width, height, channels;
        unsigned char* data =
            stbi_load(pathString.c_str(), &width, &height, &channels, 4);

        if (data) {
            int targetSize = static_cast<int>(this->THUMBNAIL_SIZE);
            unsigned char* resizedPixels =
                new unsigned char[targetSize * targetSize * 4];

            stbir_resize_uint8_srgb(data, width, height, 0, resizedPixels,
                                    targetSize, targetSize, 0, STBIR_RGBA);
            stbi_image_free(data);

            std::lock_guard<std::mutex> lock(resultsMutex);
            pendingResults.push_back(
                {pathString, resizedPixels, targetSize, targetSize});
        } else {
            std::lock_guard<std::mutex> lock(resultsMutex);
            pendingResults.push_back({pathString, nullptr, 0, 0});
        }
    }).detach();
}

void FilesPanel::ProcessPendingThumbnails() {
    std::lock_guard<std::mutex> lock(resultsMutex);
    for (auto& result : pendingResults) {
        if (result.pixels) {
            thumbnails[result.path] = std::unique_ptr<Texture2D>(
                Texture2D::Create(result.pixels, result.width, result.height,
                                  Texture::ColorTextureRGBA));
            delete[] result.pixels;
        } else {
            thumbnails[result.path] = nullptr;
        }
        loadingPaths.erase(result.path);
    }
    pendingResults.clear();
}

void FilesPanel::GetIcons() {
    if (!this->folderIcon)
        folderIcon.reset(Texture2D::Load("./res/editor/icons/folder.png",
                                         Texture::ColorTextureRGBA, false));
    if (!this->fileIcon)
        fileIcon.reset(Texture2D::Load("./res/editor/icons/file.png",
                                       Texture::ColorTextureRGBA, false));
    if (!this->modelIcon)
        modelIcon.reset(Texture2D::Load("./res/editor/icons/model.png",
                                        Texture::ColorTextureRGBA, false));
    if (!this->audioIcon)
        audioIcon.reset(Texture2D::Load("./res/editor/icons/audio_file.png",
                                        Texture::ColorTextureRGBA, false));
    if (!this->codeIcon)
        codeIcon.reset(Texture2D::Load("./res/editor/icons/"
                                       "code_file.png",
                                       Texture::ColorTextureRGBA, false));
}
} // namespace Editor
