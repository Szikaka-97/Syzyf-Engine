#pragma once

#include "GameObjectSystem.h"
#include "ui/objects/UiInteractable.h"

class UiInteractableSystem : public GameObjectSystem<UiInteractable> {
public:
    UiInteractableSystem(Scene* scene);

    virtual void OnPreUpdate() override;
};
