#include "text/Font.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <Texture.h>

using json = nlohmann::json;

Font* Font::Load(const std::filesystem::path& jsonPath, Texture2D* atlasTexture, bool useMsdf) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        spdlog::error("Failed to load font: {}", jsonPath.string());

        return nullptr;
    }

    json j;
    file >> j;

    Font* font = new Font();
    font->useMsdf = useMsdf;
    font->atlasTexture = atlasTexture;

    if (!atlasTexture) {
        std::filesystem::path atlasPath = jsonPath;
        atlasPath.replace_extension("png");

        TextureParams fontTextureParams = {
            .channels = TextureChannels::RGB,
            .colorSpace = TextureColor::Linear,
            .format = TextureFormat::Ubyte,
            .wrapU = TextureWrap::Clamp,
            .wrapV = TextureWrap::Clamp,
            .minFilter = TextureFilter::Linear,
            .magFilter = TextureFilter::Linear
        };

        font->atlasTexture = ResourceDatabase::Global->Get<Texture2D>(atlasPath, fontTextureParams);
    }

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

    font->path = jsonPath;

    spdlog::info("loading font: {}", jsonPath.string());

    return font;
}

fs::path Font::GetPath() const {
    return this->path;
}
uint64_t Font::GetHash() const {
    return 0;
}

nlohmann::json Font::Serialize() const {
    // nlohmann::json data;

    // if (this->atlasTexture) {
    //     data["atlasTexture"] = this->atlasTexture->GetPath();
    // }
}
void Font::Deserialize(const nlohmann::json& data) {
    // if (data.contains("atlasTexture")) {
    //     this->atlasTexture = ResourceDatabase::Global->Get<Texture2D>(data["atlasTexture"]);
    // }
}