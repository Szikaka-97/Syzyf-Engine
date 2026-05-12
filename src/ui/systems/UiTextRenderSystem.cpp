#include "ui/systems/UiTextRenderSystem.h"
#include "ui/objects/UiLayout.h"
#include "ui/systems/UiLayoutSystem.h"
#include "ui/objects/UiText.h"
#include "Graphics.h"
#include "Texture.h"

UiTextRenderSystem::UiTextRenderSystem(Scene* scene) : GameObjectSystem<UiText>(scene) {}

void UiTextRenderSystem::OnPreRender() {
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

        Font* font = text->font;
        const float textureWidth = static_cast<float>(font->atlasTexture->GetWidth());
        const float textureHeight = static_cast<float>(font->atlasTexture->GetHeight());

        const float scale = (text->fontSize * scaleFactor) / static_cast<float>(font->emSize);

        float cursorX = layout->finalRectangle.x;
        float cursorY = layout->finalRectangle.y + (static_cast<float>(font->ascender) * scale);

        for (const char c : text->text) {
            if (c == '\n') {
                cursorX = layout->finalRectangle.x;
                cursorY += static_cast<float>(font->lineHeight) * scale;
                continue;
            }

            auto it = font->glyphs.find(static_cast<uint32_t>(c));
            if (it == font->glyphs.end()) {
                spdlog::warn("UiTextRenderSystem::OnPreRender: Failed to find the required glyph in the atlas");
                continue;
            }

            const MsdfGlyph& glyph = it->second;

            if (glyph.planeBounds.z > glyph.planeBounds.x) {
                float x0 = cursorX + (glyph.planeBounds.x * scale);
                float y0 = cursorY - (glyph.planeBounds.y * scale);
                float x1 = cursorX + (glyph.planeBounds.z * scale);
                float y1 = cursorY - (glyph.planeBounds.w * scale);

                glm::vec4 quadRectangle(x0, y0, x1 - x0, y1 - y0);

                float u0 = glyph.atlasBounds.x / textureWidth;
                float v0 = glyph.atlasBounds.y / textureHeight;
                float u1 = glyph.atlasBounds.z / textureWidth;
                float v1 = glyph.atlasBounds.w / textureHeight;

                glm::vec4 uvRectangle(u0, v0, u1 - u0, v1 - v0);

                this->GetScene()->GetGraphics()->DrawUiText(quadRectangle, layout->zIndex, text->color, font->atlasTexture, uvRectangle, static_cast<float>(font->distanceRange));
            }

            cursorX += static_cast<float>(glyph.advance) * scale;
        }
    }
}
