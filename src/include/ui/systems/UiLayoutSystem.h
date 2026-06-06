#pragma once

#include "ui/objects/UiLayout.h"

#include "GameObjectSystem.h"

class UiLayoutSystem : public GameObjectSystem<UiLayout> {
public:
    static constexpr glm::uvec2 VIRTUAL_RESOLUTION  = { 1920, 1080 };

    UiLayoutSystem(Scene* scene);

    // Preupdate doesnt work for whatever reason, fix
    virtual void OnPreRender() override;
private:
    glm::vec2 CalculateLocalAnchorPosition(AnchorPoint anchor, glm::vec2 parentSize, glm::vec2 childSize, glm::vec2 offset);
};
