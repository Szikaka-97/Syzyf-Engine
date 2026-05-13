#pragma once

#include "SceneComponent.h"

// Sole purpose of this is to add the ui systems so it's not necessary to have to add them individually
class UiSystem : public SceneComponent {
public:
    UiSystem(Scene* scene);    
};
