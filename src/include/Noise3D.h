#pragma once

#include "FastNoiseLite.h"
#include "Texture.h"

namespace Noise3D {
inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline float GetSeamlessNoise(FastNoiseLite& noise, float x, float y, float z, float size) {
    float u = x / size;
    float v = y / size;
    float w = z / size;

    float wx = u * u * (3.0f - 2.0f * u);
    float wy = v * v * (3.0f - 2.0f * v);
    float wz = w * w * (3.0f - 2.0f * w);

    float n000 = noise.GetNoise(x, y, z);
    float n100 = noise.GetNoise(x - size, y, z);
    float n010 = noise.GetNoise(x, y - size, z);
    float n110 = noise.GetNoise(x - size, y - size, z);
    float n001 = noise.GetNoise(x, y, z - size);
    float n101 = noise.GetNoise(x - size, y, z - size);
    float n011 = noise.GetNoise(x, y - size, z - size);
    float n111 = noise.GetNoise(x - size, y - size, z - size);

    float nx00 = lerp(n000, n100, wx);
    float nx10 = lerp(n010, n110, wx);
    float nx01 = lerp(n001, n101, wx);
    float nx11 = lerp(n011, n111, wx);

    float nxy0 = lerp(nx00, nx10, wy);
    float nxy1 = lerp(nx01, nx11, wy);

    return lerp(nxy0, nxy1, wz);
}

inline Texture3D* Create3DNoiseTexture(FastNoiseLite& noise, float size, bool seamless = false) {
  spdlog::info("Noise3D: Generating a {}x{}x{} 3D noise texture...", size, size, size);
  int bufferSize = size * size * size;
  unsigned char* noiseData = new unsigned char[bufferSize];

  int index = 0;
  for (int z = 0; z < size; z++) {
    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        float rawNoise;
        if (seamless) {
          rawNoise = GetSeamlessNoise(noise, (float)x, (float)y, (float)z, (float)size);
        } else {
          rawNoise = noise.GetNoise((float)x, (float)y, (float)z); 
        }

        float mappedNoise = (rawNoise + 1.0f) * 0.5f;

        mappedNoise = std::clamp(mappedNoise, 0.0f, 1.0f);

        noiseData[index++] = (unsigned char)(mappedNoise * 255.0f);
      }
    }
  }
  TextureParams params;
  params.channels = TextureChannels::Grayscale;
  params.format = TextureFormat::Ubyte;
  params.colorSpace = TextureColor::Linear;

  params.wrapU = TextureWrap::Repeat;
  params.wrapV = TextureWrap::Repeat;
  params.wrapW = TextureWrap::Repeat;

  params.minFilter = TextureFilter::LinearMipmapLinear;
  params.magFilter = TextureFilter::Linear;
  Texture3D* noiseTexture = Texture3D::Create(noiseData, size, size, size, params);
  delete[] noiseData;

  return noiseTexture;
}
}
