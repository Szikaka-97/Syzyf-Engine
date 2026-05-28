#include "text/Font.h"

#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;

Font* Font::Load(const std::filesystem::path& jsonPath, Texture2D* atlasTexture, bool useMsdf) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) return nullptr;

    json j;
    file >> j;

    Font* font = new Font();
    font->useMsdf = useMsdf;
    font->atlasTexture = atlasTexture;

    if (font->useMsdf)
        font->distanceRange = j["atlas"]["distanceRange"];
    else
        font->distanceRange = 0;
    font->emSize = j["metrics"]["emSize"];
    font->lineHeight = j["metrics"]["lineHeight"];
    font->ascender = j["metrics"]["ascender"];
    font->descender = j["metrics"]["descender"];

    for (const auto& glyphJson : j["glyphs"]) {
        Glyph glyph;
        glyph.unicode = glyphJson["unicode"];
        glyph.advance = glyphJson["advance"];

        if (glyphJson.contains("planeBounds")) {
            glyph.planeBounds = glm::vec4(
                glyphJson["planeBounds"]["left"],
                glyphJson["planeBounds"]["bottom"],
                glyphJson["planeBounds"]["right"],
                glyphJson["planeBounds"]["top"]
            );
        }

        if (glyphJson.contains("atlasBounds")) {
            glyph.atlasBounds = glm::vec4(
                glyphJson["atlasBounds"]["left"],
                glyphJson["atlasBounds"]["bottom"],
                glyphJson["atlasBounds"]["right"],
                glyphJson["atlasBounds"]["top"]
            );
        }

        font->glyphs[glyph.unicode] = glyph;
    }

    return font;
}
