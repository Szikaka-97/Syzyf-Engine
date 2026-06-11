#pragma once

#include <FastNoiseLite.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

class Texture2D;

namespace Editor {
class Context;

struct ColorKey {
    float position;
    glm::vec4 color;

    bool operator<(const ColorKey& other) const {
        return position < other.position;
    }
};

class TextureToolPanel {
  private:
    FastNoiseLite noise;
    Texture2D* generatedTexture = nullptr;

    // ---- Noise Settings ----
    // General Settings
    int resolution = 256;
    int noiseType = 0;
    int seed = 2137;
    float frequency = 0.01f;
    bool normalize = true;

    // Fractal Settings
    int fractalType = FastNoiseLite::FractalType_None;
    int octaves = 3;
    float lacunarity = 2.0f;
    float gain = 0.5f;
    float weightedStrength = 0.0f;
    float pingPongStrength = 2.0f;

    // Cellular Settings
    int cellularDistanceFunction =
        FastNoiseLite::CellularDistanceFunction_EuclideanSq;
    int cellularReturnType = FastNoiseLite::CellularReturnType_Distance;
    float cellularJitter = 1.0f;

    // Texture Data
    std::vector<float> textureData;
    std::uint32_t previewTextureId = 0;
    bool needsUpdate = true;

    // ---- Gradient Stuff ----
    int gradientType = 0; // 0 = 1D, 1 = 2D
    int gradientDirection = 0;
    int gradientResolution = 256;
    bool gradientNeedsUpdate = true;

    // 1D Gradient
    std::vector<ColorKey> gradientKeys = {
        {0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)},
        {1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)}};

    std::vector<std::uint8_t> gradientTextureData;
    Texture2D* generatedGradientTexture = nullptr;
    std::uint32_t gradientPreviewId = 0;

  public:
    TextureToolPanel();
    void Draw(Context& context);

  private:
    void DrawNoiseTab(Context& context);
    void DrawGradientTab(Context& context);

    void GenerateNoiseTexture();
    void GenerateGradientTexture();

    void SaveNoiseToFile(const std::string& path);
    void SaveGradientToFile(const std::string& path);
};
} // namespace Editor
