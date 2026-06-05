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
                this->SaveTextureToFile(path);
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

    ImGui::End();
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

void TextureToolPanel::SaveTextureToFile(const std::string& path) {
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
} // namespace Editor
