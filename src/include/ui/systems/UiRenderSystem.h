#pragma once

#include "GameObjectSystem.h"

#include "ui/objects/UiVisual.h"

class UiText;
class UiLayout;

class UiRenderSystem : public GameObjectSystem<UiVisual> {
public:
    UiRenderSystem(Scene* scene);

    void OnPreRender() override;
};
