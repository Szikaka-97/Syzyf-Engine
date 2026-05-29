#include "ui/systems/UiTextRenderSystem.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiVisual.h"
#include "ui/systems/UiLayoutSystem.h"
#include "ui/objects/UiText.h"
#include "Graphics.h"
#include "Texture.h"
#include <optional>

UiTextRenderSystem::UiTextRenderSystem(Scene* scene) : GameObjectSystem<UiText>(scene) {}

void UiTextRenderSystem::OnPreRender() {
    struct TextLine {
        float length;
        int numChars;
    };

    glm::vec2 resolution = this->GetScene()->GetGraphics()->GetScreenResolution();
    float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

    for (auto* text : IterateObjects()) {
        UiLayout* layout = text->GetObject<UiLayout>();
        if (!layout) {
            spdlog::warn("UiTextRenderSystem::OnPreRender: Tried drawing text without a UiLayout component");
            continue;
        }

        if (!text->font || text->text.empty()) {
            spdlog::warn("UiTextRenderSystem::OnPreRender: Tried drawing text with an incomplete UiText component");
            continue;
        }

        int charsInCurrentLine = 0;

        std::vector<TextLine> lineLenghts;
        float longestLine = 0;

        Font* font = text->font;
        const float textureWidth = static_cast<float>(font->atlasTexture->GetWidth());
        const float textureHeight = static_cast<float>(font->atlasTexture->GetHeight());

        const float scale = (text->fontSize * scaleFactor) / static_cast<float>(font->emSize);

        const float localStartX = -(layout->size.x * scaleFactor) * 0.5f;
        const float localStartY = -(layout->size.y * scaleFactor) * 0.5f + (static_cast<float>(font->ascender) * scale);

        float cursorX = localStartX;
        float cursorY = localStartY;

        float xOffset = 0.0f;
        float yOffset = 0.0f;

        glm::mat4 baseWorldMatrix = text->GlobalTransform().Value();

        int startingChar = 0;

        for (char c : text->text) {
            if (c == '\n') {
                lineLenghts.push_back({
                    xOffset,
                    charsInCurrentLine + 1
                });

                if (xOffset > longestLine) {
                    longestLine = xOffset;
                }

                startingChar += charsInCurrentLine;

                xOffset = 0.0f;
                charsInCurrentLine = 0;
            }
            else {
                charsInCurrentLine++;
                
                if (font->glyphs.find(c) == font->glyphs.end()) continue;

                const Glyph& glyph = font->glyphs[c];

                if (c == ' ' && text->maxWidth.has_value()) {
                    float scaledMaxWidth = text->maxWidth.value() * scaleFactor;
                    float nextWordWidth = MeasureWordWidth(text, startingChar + charsInCurrentLine, scale);
                    float spaceAdvance = static_cast<float>(glyph.advance) * scale;

                    if (xOffset + spaceAdvance + nextWordWidth > scaledMaxWidth) {
                        lineLenghts.push_back({
                            xOffset,
                            charsInCurrentLine
                        });

                        xOffset = 0.0f;
                        charsInCurrentLine = 0;
                    }
                }

                xOffset += glyph.advance * scale;
            }
        }

        lineLenghts.push_back({
            xOffset,
            charsInCurrentLine
        });

        if (xOffset > longestLine) {
            longestLine = xOffset;
        }

        startingChar = 0;

        for (auto line : lineLenghts) {
            if (text->alignment == TextAlignment::Left) {
                xOffset = 0;
            }
            else if (text->alignment == TextAlignment::Middle) {
                xOffset = (longestLine - line.length) * 0.5f;
            }
            else {
                xOffset = longestLine - line.length;
            }

            for (int i = 0; i < line.numChars; i++) {
                char c = text->text[startingChar + i];

                if (c == '\n') {
                    continue;
                }

                auto it = font->glyphs.find(static_cast<uint32_t>(c));
                if (it == font->glyphs.end()) {
                    spdlog::warn("UiTextRenderSystem::OnPreRender: Failed to find the required glyph in the atlas: '{}' ({:x})", c, (int) c);
                    continue;
                }

                const Glyph& glyph = it->second;
                
                if (glyph.planeBounds.z > glyph.planeBounds.x) {
                    float x0 = cursorX + xOffset + (glyph.planeBounds.x * scale);
                    float y0 = cursorY + yOffset - (glyph.planeBounds.y * scale);
                    float x1 = cursorX + xOffset + (glyph.planeBounds.z * scale);
                    float y1 = cursorY + yOffset - (glyph.planeBounds.w * scale);

                    const float width = x1 - x0;
                    const float height = y1 - y0;

                    const float localCenterX = x0 + (width * 0.5f);
                    const float localCenterY = y0 + (height * 0.5f);

                    glm::mat4 letterMatrix = glm::translate(baseWorldMatrix, glm::vec3(localCenterX, localCenterY, 0.0f));
                    glm::vec2 letterSize(width, height);

                    const float u0 = glyph.atlasBounds.x / textureWidth;
                    const float v0 = glyph.atlasBounds.y / textureHeight;
                    const float u1 = glyph.atlasBounds.z / textureWidth;
                    const float v1 = glyph.atlasBounds.w / textureHeight;

                    glm::vec4 uvRectangle(u0, v0, u1 - u0, v1 -v0);

                    auto* visual = text->GetObject<UiVisual>();
                    auto clipRectangle = visual ? visual->clipRectangle : std::nullopt; 

                    this->GetScene()->GetGraphics()->DrawUiText(letterMatrix, letterSize, layout->zIndex, text->color, font->atlasTexture, uvRectangle, static_cast<float>(font->distanceRange), font->useMsdf, clipRectangle);
                }

                xOffset += static_cast<float>(glyph.advance) * scale;
            }

            yOffset += font->lineHeight * scale;

            startingChar += line.numChars;
        }
    }
}

float UiTextRenderSystem::MeasureWordWidth(const UiText* text, size_t startIndex, float scale) {
    float width = 0.0f;
    for (size_t i = startIndex; i < text->text.length(); ++i) {
        char c = text->text[i];
        if (c == ' ' || c == '\n') break;

        auto glyphIt = text->font->glyphs.find(static_cast<uint32_t>(c));
        if (glyphIt != text->font->glyphs.end()) {
            width += static_cast<float>(glyphIt->second.advance) * scale;
        }
    }
    return width;
}
