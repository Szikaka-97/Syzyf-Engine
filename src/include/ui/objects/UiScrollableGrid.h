#pragma once

#include "GameObject.h"
#include "Debug.h"

class UiScrollableGrid : public GameObject, public ImGuiDrawable {
public:
    int columns = 3;
    glm::ivec2 cellSize = {100, 100};
    glm::ivec2 spacing = {15, 15};

    float scrollY = 0.0f;
    float targetScrollY = 0.0f;
    float scrollSpeed = 10.0f;

    int selectedIndex = 0;

    void Update();

    void DrawImGui();
};
