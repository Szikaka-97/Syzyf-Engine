#pragma once

#include "GameObjectSystem.h"
#include "ui/UiInteractable.h"

class UiInteractableSystem : public GameObjectSystem<UiInteractable> {
public:
    UiInteractableSystem(Scene* scene);

    virtual void OnPreUpdate() override;
};
