#pragma once

#include "GameObject.h"
#include <Scene.h>
#include <Resources.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiVisual.h>

class InGameUi : public GameObject {
public:
    InGameUi() = default;

    void Awake() {
        Scene* mainScene = GetScene();
        SceneNode* uiRoot = GetNode();

        const int width = 413;
        const int height = 121;

        SceneNode* healthNode = mainScene->GetOrCreateNode(uiRoot, "Health UI");
        healthNode->AddObjectIfMissing<UiLayout>(glm::uvec2(width, height), glm::uvec2(40, -40), 0, AnchorPoint::BottomLeft);
        
        Texture2D* healthBackgroundTexture = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/life_background.png", Texture2D::ColorTextureRGBA);
            
        healthNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), healthBackgroundTexture);

        SceneNode* healthVialNode = mainScene->GetOrCreateNode(healthNode, "Health Vial");
        healthVialNode->AddObjectIfMissing<UiLayout>(glm::uvec2(width, height), glm::uvec2(0, 0), 1, AnchorPoint::Center);
        
        Texture2D* healthForegroundTexture = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/life_foreground.png", Texture2D::ColorTextureRGBA);
            
        healthVialNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), healthForegroundTexture);
    }
};
