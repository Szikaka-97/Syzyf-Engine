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

        const float localStartX = -(layout->size.x * scaleFactor) * 0.5f;
        const float localStartY = -(layout->size.y * scaleFactor) * 0.5f + (static_cast<float>(font->ascender) * scale);

        float cursorX = localStartX;
        float cursorY = localStartY;

        glm::mat4 baseWorldMatrix = text->GlobalTransform().Value();

        for (const char c : text->text) {
            if (c == '\n') {
                cursorX = localStartX;
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

                this->GetScene()->GetGraphics()->DrawUiText(letterMatrix, letterSize, layout->zIndex, text->color, font->atlasTexture, uvRectangle, static_cast<float>(font->distanceRange));
            }

            cursorX += static_cast<float>(glyph.advance) * scale;
        }
    }
}
