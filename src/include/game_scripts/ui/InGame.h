#pragma once

#include "GameObject.h"
#include <Scene.h>
#include <Resources.h>
#include <memory>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiVisual.h>
#include <game_scripts/ui/UiHealthBar.h>

class InGameUi : public GameObject {
public:
    InGameUi() = default;

    void Awake() {
        Scene* mainScene = GetScene();
        SceneNode* uiRoot = GetNode();

        const int width = 500;
        const int height = 146;

        SceneNode* healthNode = mainScene->GetOrCreateNode(uiRoot, "Health UI");
        healthNode->AddObjectIfMissing<UiLayout>(glm::uvec2(width, height), glm::uvec2(40, -40), 0, AnchorPoint::BottomLeft);
        
        Texture2D* healthBackgroundTexture = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/life_background.png", Texture2D::ColorTextureRGBA);
            
        healthNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), healthBackgroundTexture);

        // Health Bar Fill
        ShaderProgram* healthBarProgram = ShaderProgram::Build()
            .WithVertexShader("./res/shaders/ui/ui.vert")
            .WithPixelShader("./res/shaders/ui/custom/health_bar.frag")
            .Link();

        auto healthBarMaterial = std::make_shared<Material>(healthBarProgram);
        healthBarMaterial->SetValue("percent", 1.0f);

        SceneNode* healthFillNode = mainScene->GetOrCreateNode(healthNode, "Health Fill");
        healthFillNode->AddObjectIfMissing<UiLayout>(glm::uvec2(320, 60), glm::uvec2(-25, 2), 1, AnchorPoint::Center);

        Texture2D* healthFillTexture = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/health_fill.png", Texture2D::ColorTextureRGBA
        );

        auto* fillVisual = healthFillNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), healthFillTexture);
        fillVisual->customMaterial = healthBarMaterial.get(); // hmm

        auto* healthBarLogic = healthFillNode->AddObjectIfMissing<UiHealthBar>();
        healthBarLogic->material = healthBarMaterial;

        SceneNode* healthVialNode = mainScene->GetOrCreateNode(healthNode, "Health Vial");
        healthVialNode->AddObjectIfMissing<UiLayout>(glm::uvec2(width, height), glm::uvec2(0, 0), 2, AnchorPoint::Center);
        
        Texture2D* healthForegroundTexture = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/life_foreground.png", Texture2D::ColorTextureRGBA);
            
        healthVialNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), healthForegroundTexture);
    }
};
