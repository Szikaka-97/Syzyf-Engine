#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "Material.h"

#include <imgui.h>

class UiCircularBar : public GameObject, public ImGuiDrawable {
public:
    float value = 45.0f;
    float maxValue = 100.0f;

    Material* material = nullptr;

    void Update() {
        if (!material) return;

        if (value > maxValue) value = maxValue;
        if (value < 0.0f) value = 0.0f;

        float percentage = value / maxValue;
       
        material->SetValue("level", percentage);
    }

    void DrawImGui() {
        ImGui::SliderFloat("Value", &this->value, 0.0, 100.0f);
    }
};
