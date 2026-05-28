#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiVisual.h"

#include <imgui.h>

class UiHealthBar : public GameObject, public ImGuiDrawable {
public:
    UiLayout* fillLayout = nullptr;
    int maxWidth = 0;
    float maxHealth = 100.0f;

    SceneNode* playerNode = nullptr;
    SceneNode* mockEnemyNode = nullptr;
    float distanceToAppear = 22.0f;

    UiVisual* visual = nullptr;
    UiVisual* fillVisual = nullptr;
    UiVisual* bgVisual = nullptr; //wtf

public:
    void Awake() {
        this->visual = this->GetObject<UiVisual>();
    }

    void Update() {
        if (!mockEnemyNode || !playerNode || !visual || !fillVisual) return;

        float distance = glm::distance(
            this->playerNode->GlobalTransform().Position().Value(),
            this->mockEnemyNode->GlobalTransform().Position().Value()
        );

        if (distance <= this->distanceToAppear) {
            if (!this->visual->IsEnabled()) {
                this->visual->SetEnabled(true);
                this->fillVisual->SetEnabled(true);
                this->bgVisual->SetEnabled(true);
            }
        } else if (this->visual->IsEnabled()) {
            this->visual->SetEnabled(false);
            this->fillVisual->SetEnabled(false);
            this->bgVisual->SetEnabled(false);
        }
    }

    void SetHealth(float currentHealth) {
        if (!fillLayout) return;

        float percentage = std::clamp(currentHealth / maxHealth, 0.0f, 1.0f);
        fillLayout->size.x = static_cast<int>(maxWidth * percentage);
    }

    void DrawImGui() {
        ImGui::InputInt("Max Width", &this->maxWidth);

        float health = 100.0f;
        if (ImGui::InputFloat("Health", &health)) {
            this->SetHealth(health);
        }

        ImGui::InputFloat("Distance", &this->distanceToAppear);
    }
};
