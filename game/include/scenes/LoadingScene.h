#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/systems/UiSystem.h"
#include "BaseScene.h"

namespace LoadingScene {

class LoadingController : public GameObject {
  private:
    int framesPassed = 0;

  public:
    LoadingController() = default;

    void Update() {
        if (framesPassed > 2) {
            Application::Get()->RequestSceneBuild(
                // [](Scene* s) { BaseScene::InitScene(*s); });
                [](Scene* s) { BaseScene::InitScene(*s); });
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

    TextureParams fontParams = {.channels = TextureChannels::RGB,
                                .colorSpace = TextureColor::Linear,
                                .format = TextureFormat::Ubyte,
                                .wrapU = TextureWrap::Clamp,
                                .wrapV = TextureWrap::Clamp,
                                .minFilter = TextureFilter::Linear,
                                .magFilter = TextureFilter::Linear};
    Texture2D* fontAtlas = mainScene.Resources()->Get<Texture2D>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.png", fontParams);
    Font* font = mainScene.Resources()->Get<Font>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.json", fontAtlas);

    SceneNode* textNode = mainScene.CreateNode("Loading Text");
    textNode->AddObject<UiLayout>(glm::uvec2(400, 50), glm::ivec2(0, 0), 0,
                                  AnchorPoint::Center);
    auto* text = textNode->AddObject<UiText>("Loading...", font);
    text->fontSize = 32.0f;

    SceneNode* logicNode = mainScene.CreateNode("Loading Logic");
    logicNode->AddObject<LoadingController>();
}
} // namespace LoadingScene
