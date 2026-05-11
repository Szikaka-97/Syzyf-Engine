#pragma once

#include "GameObjectSystem.h"

#include "ui/UiVisual.h"

class UiRenderSystem : public GameObjectSystem<UiVisual> {
public:
    UiRenderSystem(Scene* scene);

    void OnPreRender() override;
};
