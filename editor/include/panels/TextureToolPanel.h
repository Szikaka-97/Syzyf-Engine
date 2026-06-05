#pragma once

#include <FastNoiseLite.h>

#include <cstdint>
#include <string>
#include <vector>

class Texture2D;

namespace Editor {
class Context;

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

  public:
    TextureToolPanel();
    void Draw(Context& context);

  private:
    void GenerateNoiseTexture();
    void SaveTextureToFile(const std::string& path);
};
} // namespace Editor
