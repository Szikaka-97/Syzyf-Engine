#pragma once

#include "ui/objects/UiLayout.h"

#include "GameObjectSystem.h"

class UiLayoutSystem : public GameObjectSystem<UiLayout> {
public:
    static constexpr glm::uvec2 VIRTUAL_RESOLUTION  = { 1920, 1080 };

    UiLayoutSystem(Scene* scene);

    // Preupdate doesnt work for whatever reason, fix
    virtual void OnPreRender() override;
};
