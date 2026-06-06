#pragma once

#include "GameObjectSystem.h"
#include "ui/objects/UiText.h"

class UiTextRenderSystem : public GameObjectSystem<UiText> {
public:
    UiTextRenderSystem(Scene* scene);

    void OnPreRender() override;
private:
    float MeasureWordWidth(const UiText* text, size_t startIndex, float scale);
};
