#include "panels/FilesPanel.h"

#include <Texture.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <sstream>
#include <stb_image.h>
#include <stb_image_resize2.h>

namespace fs = std::filesystem;

namespace Editor {
void FilesPanel::Draw() {
    this->ProcessPendingThumbnails();

    this->GetIcons();

    ImGui::Begin("Files");

    bool canGoBack =
        this->currentDirectory != "./res/" && this->currentDirectory != "./res";
    if (!canGoBack)
        ImGui::BeginDisabled();
    if (ImGui::Button("Back")) {
        this->currentDirectory = this->currentDirectory.parent_path();
    }
    if (!canGoBack)
        ImGui::EndDisabled();
    ImGui::SameLine();

    float searchWidth = 200.0f;
    float rightEdge = ImGui::GetWindowContentRegionMax().x;

    if (rightEdge - searchWidth > ImGui::GetCursorPosX()) {
        ImGui::SetCursorPosX(rightEdge - searchWidth);
    }

    ImGui::SetNextItemWidth(searchWidth);
    ImGui::InputTextWithHint("##FileSearch", "Search...", this->searchBuffer,
                             sizeof(this->searchBuffer));

    ImGui::Separator();

    std::string searchString = this->searchBuffer;
    std::transform(searchString.begin(), searchString.end(),
                   searchString.begin(), ::tolower);

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / this->CELL_SIZE);
    if (columnCount < 1) {
        columnCount = 1;
    }

    if (ImGui::BeginTable("FileBrowserGrid", columnCount)) {
        if (fs::exists(this->currentDirectory)) {

            std::vector<fs::directory_entry> directoryEntries;
            for (const auto& entry :
                 fs::directory_iterator(this->currentDirectory)) {
                directoryEntries.push_back(entry);
            }

            std::sort(
                directoryEntries.begin(), directoryEntries.end(),
                [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    bool aIsDir = a.is_directory();
                    bool bIsDir = b.is_directory();

                    if (aIsDir && !bIsDir)
                        return true;
                    if (!aIsDir && bIsDir)
                        return false;

                    return a.path().filename().string() <
                           b.path().filename().string();
                });

            for (auto& directoryEntry : directoryEntries) {
                const auto& path = directoryEntry.path();
                std::string filenameString = path.filename().string();

                if (!searchString.empty()) {
                    std::string lowerFilename = filenameString;
                    std::transform(lowerFilename.begin(), lowerFilename.end(),
                                   lowerFilename.begin(), ::tolower);

                    if (lowerFilename.find(searchString) == std::string::npos) {
                        continue;
                    }
                }

                ImGui::TableNextColumn();

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
                    } else {
                        this->showPreviewPopup = true;
                        this->previewFilePath = path.filename().string();

                        std::string extension = path.extension().string();
                        std::transform(extension.begin(), extension.end(),
                                       extension.begin(), ::tolower);

                        if (extension == ".png" || extension == ".jpg" ||
                            extension == ".jpeg" || extension == ".bmp" ||
                            extension == ".hdr") {
                            this->previewTexture.reset(Texture2D::Load(
                                path.string(), Texture::ColorTextureRGBA,
                                false));
                            this->previewTextContent.clear();
                        } else if (extension == ".vert" ||
                                   extension == ".frag" ||
                                   extension == ".comp" ||
                                   extension == ".geom" ||
                                   extension == ".tess_eval" ||
                                   extension == ".tess_ctrl" ||
                                   extension == ".txt") {
                            std::ifstream fileStream(path);
                            if (fileStream.is_open()) {
                                std::stringstream buffer;
                                buffer << fileStream.rdbuf();
                                this->previewTextContent = buffer.str();
                            } else {
                                this->previewTextContent =
                                    "Failed to open the file.";
                            }
                        } else {
                            this->previewTexture.reset();
                            this->showPreviewPopup = false;
                        }
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

    this->DrawPreviewPopup();

    ImGui::End();
}

void FilesPanel::DrawPreviewPopup() {
    // Preview popup
    if (this->showPreviewPopup) {
        ImGui::OpenPopup("File Preview");
        this->showPreviewPopup = false;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("File Preview", nullptr,
                               ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("Previewing: %s", this->previewFilePath.c_str());
        ImGui::Separator();

        if (this->previewTexture) {
            ImVec2 availableSize = ImGui::GetContentRegionAvail();
            availableSize.y -= 30;

            float aspect =
                static_cast<float>(this->previewTexture->GetWidth()) /
                static_cast<float>(this->previewTexture->GetHeight());
            ImVec2 imageSize =
                ImVec2(availableSize.x, availableSize.x / aspect);
            if (imageSize.y > availableSize.y) {
                imageSize.y = availableSize.y;
                imageSize.x = availableSize.y * aspect;
            }

            ImGui::Image(
                (ImTextureID)(intptr_t)this->previewTexture->GetHandle(),
                imageSize);
        } else {
            ImVec2 availableSize = ImGui::GetContentRegionAvail();
            availableSize.y -= 30;

            ImGui::InputTextMultiline("##source", this->previewTextContent.data(),
                                      this->previewTextContent.size() + 1,
                                      availableSize,
                                      ImGuiInputTextFlags_ReadOnly |
                                          ImGuiInputTextFlags_AllowTabInput);
        }

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            this->previewTexture.reset();
            this->previewTextContent.clear();
        }
        ImGui::EndPopup();
    }
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
