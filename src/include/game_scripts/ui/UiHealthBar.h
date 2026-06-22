#pragma once

#include "Debug.h"
#include "Material.h"
#include "TimeSystem.h"
#include "game_scripts/PlayerController.h"

class UiHealthBar : public GameObject, public ImGuiDrawable {
public:
    float currentPercentage = 1.0f;
    float maxHealth = 100.0f;
    float lerpSpeed = 5.0f;

    std::shared_ptr<Material> material;

    void Update() {
        if (!material) return;

        float targetPercentage = 0.0f;

        PlayerController* player = PlayerController::Instance();
        if (player) {
            float health = player->GetHealth();

            targetPercentage = health / maxHealth;

            if (targetPercentage < 0.0f) targetPercentage = 0.0f;
            if (targetPercentage > 1.0f) targetPercentage = 1.0f;
        }

        currentPercentage = glm::mix(currentPercentage, targetPercentage, lerpSpeed * Time::Delta());

        material->SetValue("percent", currentPercentage);
    }

    void DrawImGui() {
        ImGui::SliderFloat("Lerp Speed", &this->lerpSpeed, 0.1f, 20.0f);
    }
};
