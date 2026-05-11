#pragma once

#include "ui/UiLayout.h"

#include "GameObjectSystem.h"

class UiLayoutSystem : public GameObjectSystem<UiLayout> {
private:
    const glm::uvec2 VIRTUAL_RESOLUTION  = { 1920, 1080 };
public:
    UiLayoutSystem(Scene* scene);

    // Preupdate doesnt work for whatever reason, fix
    virtual void OnPreRender() override;
};
