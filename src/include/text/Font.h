#pragma once

#include "Resources.h"
#include <glm/glm.hpp>

class Texture2D;

struct Glyph {
    uint32_t unicode;
    double advance;
    glm::vec4 planeBounds;
    glm::vec4 atlasBounds;
};

class Font : public Resource {
public:
    Texture2D* atlasTexture = nullptr;
    std::unordered_map<uint32_t, Glyph> glyphs;

    double lineHeight = 0.0;
    double ascender = 0.0;
    double descender = 0.0;
    double emSize = 1.0;
    double distanceRange = 4.0;

    bool useMsdf = true;

    static Font* Load(const std::filesystem::path& jsonPath, Texture2D* atlasTexture, bool useMsdf = true);
};
