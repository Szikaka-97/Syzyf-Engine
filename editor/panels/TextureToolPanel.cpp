#include "panels/TextureToolPanel.h"
#include "EditorApplication.h"
#include "FileDialogHelpers.h"

#include <Texture.h>

#include <imgui.h>
#include <stb_image_write.h>

namespace Editor {
TextureToolPanel::TextureToolPanel() {
    noise.SetNoiseType(static_cast<FastNoiseLite::NoiseType>(noiseType));
}

void TextureToolPanel::Draw(Context& context) {
    if (!ImGui::Begin("Texture Tool")) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("TextureToolTabs")) {
        if (ImGui::BeginTabItem("Noise")) {
            this->DrawNoiseTab(context);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Gradient / Curve")) {
            this->DrawGradientTab(context);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void TextureToolPanel::DrawNoiseTab(Context& context) {
    if (this->needsUpdate) {
        GenerateNoiseTexture();
        this->needsUpdate = false;
    }

    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("TextureToolLayout", 2, tableFlags)) {
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch,
                                0.7f);
        ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_WidthStretch,
                                0.3f);

        ImGui::TableNextColumn();
        ImGui::BeginChild("PreviewPane");
        if (previewTextureId != 0) {
            ImVec2 paneAvailable = ImGui::GetContentRegionAvail();

            float displaySize = std::min(paneAvailable.x, paneAvailable.y);

            float cursorX =
                ImGui::GetCursorPosX() + (paneAvailable.x - displaySize) * 0.5f;
            float cursorY =
                ImGui::GetCursorPosY() + (paneAvailable.y - displaySize) * 0.5f;

            ImGui::SetCursorPos(ImVec2(cursorX, cursorY));
            ImGui::Image((void*)(intptr_t)previewTextureId,
                         ImVec2(displaySize, displaySize));
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("SettingsPanel");

        if (ImGui::Button("Save Texture", ImVec2(-1, 0))) {
            OpenSaveTextureDialog(context, [this](std::string path) {
                this->SaveNoiseToFile(path);
            });
        }

        if (ImGui::Button("Reset", ImVec2(-1, 0))) {
            resolution = 256;
            normalize = true;
            noiseType = FastNoiseLite::NoiseType_OpenSimplex2;
            seed = 2137;
            frequency = 0.01f;
            fractalType = FastNoiseLite::FractalType_None;
            octaves = 3;
            lacunarity = 2.0f;
            gain = 0.5f;
            weightedStrength = 0.0f;
            pingPongStrength = 2.0f;
            cellularDistanceFunction =
                FastNoiseLite::CellularDistanceFunction_EuclideanSq;
            cellularReturnType = FastNoiseLite::CellularReturnType_Distance;
            cellularJitter = 1.0f;
            needsUpdate = true;
        }

        ImGui::Spacing();
        if (ImGui::DragInt("Resolution", &resolution, 16.0f, 16, 4096))
            needsUpdate = true;
        if (ImGui::Checkbox("Normalize to [0, 1]", &normalize))
            needsUpdate = true;

        ImGui::Separator();

        // ---- NOISE ----
        // General Settings
        ImGui::Text("General Settings");
        const char* noiseTypes[] = {"OpenSimplex2", "OpenSimplex2S", "Cellular",
                                    "Perlin",       "ValueCubic",    "Value"};
        if (ImGui::Combo("Type", &noiseType, noiseTypes,
                         IM_ARRAYSIZE(noiseTypes)))
            needsUpdate = true;
        if (ImGui::DragInt("Seed", &seed))
            needsUpdate = true;
        if (ImGui::DragFloat("Frequency", &frequency, 0.001f, 0.0001f, 0.5f,
                             "%.4f"))
            needsUpdate = true;

        ImGui::Separator();

        // Fractal Settings
        ImGui::Text("Fractal Settings");
        const char* fractalTypes[] = {"None",
                                      "FBm",
                                      "Ridged",
                                      "PingPong",
                                      "DomainWarpProgressive",
                                      "DomainWarpIndependent"};
        if (ImGui::Combo("Fractal Type", &fractalType, fractalTypes,
                         IM_ARRAYSIZE(fractalTypes)))
            needsUpdate = true;

        if (fractalType != FastNoiseLite::FractalType_None) {
            if (ImGui::DragInt("Octaves", &octaves, 1.0f, 1, 10))
                needsUpdate = true;
            if (ImGui::DragFloat("Lacunarity", &lacunarity, 0.01f))
                needsUpdate = true;
            if (ImGui::DragFloat("Gain", &gain, 0.01f))
                needsUpdate = true;
            if (ImGui::DragFloat("Weighted Strength", &weightedStrength, 0.01f))
                needsUpdate = true;

            if (fractalType == FastNoiseLite::FractalType_PingPong) {
                if (ImGui::DragFloat("PingPong Strength", &pingPongStrength,
                                     0.01f))
                    needsUpdate = true;
            }
        }

        // Cellular Settings
        if (noiseType == FastNoiseLite::NoiseType_Cellular) {
            ImGui::Separator();
            ImGui::Text("Cellular Settings");
            const char* cellularDistanceFunctions[] = {
                "Euclidean", "EuclideanSq", "Manhattan", "Hybrid"};
            if (ImGui::Combo("Distance Function", &cellularDistanceFunction,
                             cellularDistanceFunctions,
                             IM_ARRAYSIZE(cellularDistanceFunctions)))
                needsUpdate = true;

            const char* cellularReturns[] = {
                "CellValue",    "Distance",     "Distance2",   "Distance2Add",
                "Distance2Sub", "Distance2Mul", "Distance2Div"};
            if (ImGui::Combo("Return Type", &cellularReturnType,
                             cellularReturns, IM_ARRAYSIZE(cellularReturns)))
                needsUpdate = true;

            if (ImGui::DragFloat("Jitter", &cellularJitter, 0.01f))
                needsUpdate = true;
        }

        ImGui::EndChild();
        ImGui::EndTable();
    }
}

void TextureToolPanel::DrawGradientTab(Context& context) {
    if (gradientNeedsUpdate) {
        GenerateGradientTexture();
        gradientNeedsUpdate = false;
    }

    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("GradientLayout", 2, tableFlags)) {
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch,
                                0.7f);
        ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_WidthStretch,
                                0.3f);

        ImGui::TableNextColumn();
        ImGui::BeginChild("GradPreviewPane");
        if (gradientPreviewId != 0) {
            ImVec2 paneAvailable = ImGui::GetContentRegionAvail();

            ImVec2 imageSize =
                (gradientType == 0)
                    ? ImVec2(paneAvailable.x, 50.0f)
                    : ImVec2(std::min(paneAvailable.x, paneAvailable.y),
                             std::min(paneAvailable.x, paneAvailable.y));

            ImGui::Image((void*)(intptr_t)gradientPreviewId, imageSize);
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("GradSettingsPanel");

        if (ImGui::Button("Save Gradient", ImVec2(-1, 0))) {
            OpenSaveTextureDialog(context, [this](std::string path) {
                this->SaveGradientToFile(path);
            });
        }

        ImGui::Spacing();
        if (ImGui::RadioButton("1D", &gradientType, 0))
            gradientNeedsUpdate = true;
        ImGui::SameLine();
        if (ImGui::RadioButton("2D", &gradientType, 1))
            gradientNeedsUpdate = true;

        if (gradientType == 1) {
            ImGui::SameLine();
            ImGui::Text("| Direction:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Horizontal", &gradientDirection, 0))
                gradientNeedsUpdate = true;
            ImGui::SameLine();
            if (ImGui::RadioButton("Vertical", &gradientDirection, 1))
                gradientNeedsUpdate = true;
        }

        if (ImGui::DragInt("Resolution", &gradientResolution, 16.0f, 16, 4096))
            gradientNeedsUpdate = true;

        ImGui::Separator();

        ImGui::Text("Color Stops");

        for (size_t i = 0; i < gradientKeys.size(); i++) {
            ImGui::PushID((int)i);
            if (ImGui::ColorEdit4("##Color", &gradientKeys[i].color.r,
                                  ImGuiColorEditFlags_NoInputs))
                gradientNeedsUpdate = true;
            ImGui::SameLine();
            if (ImGui::SliderFloat("##Pos", &gradientKeys[i].position, 0.0f,
                                   1.0f))
                gradientNeedsUpdate = true;

            if (gradientKeys.size() > 2) {
                ImGui::SameLine();
                if (ImGui::Button("-")) {
                    gradientKeys.erase(gradientKeys.begin() + i);
                    gradientNeedsUpdate = true;
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }

        if (ImGui::Button("+ Add Stop")) {
            gradientKeys.push_back({0.5f, glm::vec4(1.0f)});
            gradientNeedsUpdate = true;
        }

        ImGui::EndChild();
        ImGui::EndTable();
    }
}

void TextureToolPanel::GenerateNoiseTexture() {
    noise.SetNoiseType(static_cast<FastNoiseLite::NoiseType>(noiseType));
    noise.SetSeed(seed);
    noise.SetFrequency(frequency);

    noise.SetFractalType(static_cast<FastNoiseLite::FractalType>(fractalType));
    noise.SetFractalOctaves(octaves);
    noise.SetFractalLacunarity(lacunarity);
    noise.SetFractalGain(gain);
    noise.SetFractalWeightedStrength(weightedStrength);
    noise.SetFractalPingPongStrength(pingPongStrength);

    noise.SetCellularDistanceFunction(
        static_cast<FastNoiseLite::CellularDistanceFunction>(
            cellularDistanceFunction));
    noise.SetCellularReturnType(
        static_cast<FastNoiseLite::CellularReturnType>(cellularReturnType));
    noise.SetCellularJitter(cellularJitter);

    this->textureData.resize(resolution * resolution);

    for (int y = 0; y < this->resolution; y++) {
        for (int x = 0; x < this->resolution; x++) {
            float noiseValue = noise.GetNoise((float)x, (float)y);

            if (normalize) {
                noiseValue = (noiseValue + 1.0f) * 0.5f;
                noiseValue = std::clamp(noiseValue, 0.0f, 1.0f);
            }

            this->textureData[y * resolution + x] = noiseValue;
        }
    }

    TextureParams params;
    params.channels = TextureChannels::Grayscale;
    params.colorSpace = TextureColor::Linear;
    params.format = TextureFormat::Float;

    if (this->generatedTexture == nullptr) {
        this->generatedTexture =
            Texture2D::Create((unsigned char*)this->textureData.data(),
                              this->resolution, this->resolution, params);
        this->previewTextureId = this->generatedTexture->GetHandle();

        glBindTexture(GL_TEXTURE_2D, this->previewTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        if (this->generatedTexture->GetWidth() != resolution ||
            this->generatedTexture->GetHeight() != resolution) {
            this->generatedTexture->Resize(glm::uvec2(resolution, resolution));

            glBindTexture(GL_TEXTURE_2D, this->generatedTexture->GetHandle());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindTexture(GL_TEXTURE_2D, this->generatedTexture->GetHandle());

        GLenum internalFormat = Texture::CalcInternalFormat(params);
        GLenum format = GL_RED;
        GLenum type = GL_FLOAT;

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, this->resolution,
                     this->resolution, 0, format, type,
                     this->textureData.data());

        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void TextureToolPanel::GenerateGradientTexture() {
    int width = gradientResolution;
    int height = (gradientType == 0) ? 1 : gradientResolution;

    gradientTextureData.resize(width * height * 4);

    std::sort(gradientKeys.begin(), gradientKeys.end());

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float tx = (float)x / (width > 1 ? width - 1 : 1);
            float ty = (float)y / (height > 1 ? height - 1 : 1);

            float t = tx;
            if (gradientType == 1 && gradientDirection == 1) {
                t = ty;
            }

            glm::vec4 finalColor;
            ColorKey leftKey = gradientKeys.front();
            ColorKey rightKey = gradientKeys.back();

            for (size_t i = 0; i < gradientKeys.size() - 1; i++) {
                if (t >= gradientKeys[i].position &&
                    t <= gradientKeys[i + 1].position) {
                    leftKey = gradientKeys[i];
                    rightKey = gradientKeys[i + 1];
                    break;
                }
            }

            float range = rightKey.position - leftKey.position;
            float mixFactor =
                (range > 0.0001f) ? (t - leftKey.position) / range : 0.0f;
            finalColor = glm::mix(leftKey.color, rightKey.color, mixFactor);

            int index = (y * width + x) * 4;
            gradientTextureData[index + 0] =
                (uint8_t)(std::clamp(finalColor.r * 255.0f, 0.0f, 255.0f));
            gradientTextureData[index + 1] =
                (uint8_t)(std::clamp(finalColor.g * 255.0f, 0.0f, 255.0f));
            gradientTextureData[index + 2] =
                (uint8_t)(std::clamp(finalColor.b * 255.0f, 0.0f, 255.0f));
            gradientTextureData[index + 3] =
                (uint8_t)(std::clamp(finalColor.a * 255.0f, 0.0f, 255.0f));
        }
    }

    TextureParams params = Texture::ColorTextureRGBA;
    params.wrapU = TextureWrap::Clamp;
    params.wrapV = TextureWrap::Clamp;

    if (generatedGradientTexture == nullptr) {
        generatedGradientTexture = Texture2D::Create(gradientTextureData.data(),
                                                     width, height, params);
        gradientPreviewId = generatedGradientTexture->GetHandle();
    } else {
        if (generatedGradientTexture->GetWidth() != width ||
            generatedGradientTexture->GetHeight() != height) {
            generatedGradientTexture->Resize(glm::uvec2(width, height));
        }
        glBindTexture(GL_TEXTURE_2D, generatedGradientTexture->GetHandle());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, gradientTextureData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void TextureToolPanel::SaveNoiseToFile(const std::string& path) {
    if (path.ends_with(".hdr")) {
        stbi_write_hdr(path.c_str(), resolution, resolution, 1,
                       textureData.data());
        spdlog::info("Noise saved as HDR: {}", path);
    } else {
        std::vector<uint8_t> byteData(resolution * resolution);
        for (size_t i = 0; i < textureData.size(); i++) {
            float val = textureData[i];

            if (!normalize)
                val = (val + 1.0f) * 0.5f;

            byteData[i] =
                static_cast<uint8_t>(std::clamp(val * 255.0f, 0.0f, 255.0f));
        }
        stbi_write_png(path.c_str(), resolution, resolution, 1, byteData.data(),
                       resolution);
        spdlog::info("Noise saved as PNG map: {}", path);
    }
}

void TextureToolPanel::SaveGradientToFile(const std::string& path) {
    int width = gradientResolution;
    int height = (gradientType == 0) ? 1 : gradientResolution;

    stbi_write_png(path.c_str(), width, height, 4, gradientTextureData.data(),
                   width * 4);
    spdlog::info("Gradient saved as PNG map: {}", path);
}
} // namespace Editor
