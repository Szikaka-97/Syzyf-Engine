#pragma once

#include "Application.h"
#include "Camera.h"
#include "GameObject.h"
#include "Scene.h"
#include "TutorialThrowingRoomScene.h"
#include "scenes/MainMenuScene.h"
#include "ui/objects/UiLayout.h"
#include "ui/systems/UiSystem.h"
namespace SplashScene {

class SplashController : public GameObject {
  private:
    int framesPassed = 0;

  public:
    SplashController() {}

    void Update() {
        if (framesPassed > 2) {
            Application::Get()->RequestSceneBuild(
                [](Scene* s) { TutorialThrowingRoomScene::InitScene(*s); });
        }
        framesPassed++;
    }
};

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<UiSystem>();

    SceneNode* cameraNode = mainScene.CreateNode("Camera");
    auto* camera = cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    camera->SetAsMainCamera();

    SceneNode* uiNode = mainScene.CreateNode("Splash");
    uiNode->AddObject<UiLayout>(glm::uvec2(512, 512), glm::ivec2(0, 0), 0,
                                AnchorPoint::Center);

    TextureParams texParams = Texture::ColorTextureRGBA;
    Texture2D* splashTexture = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/lufis.jpeg", texParams);
    uiNode->AddObject<UiVisual>(glm::vec4(1.0f), splashTexture);

    SceneNode* logicNode = mainScene.CreateNode("Splash Logic");
    logicNode->AddObject<SplashController>();
}
} // namespace SplashScene
