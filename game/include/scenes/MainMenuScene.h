#pragma once

#include "Application.h"
#include "GameObject.h"
#include "TestScene.h"
#include "ui/objects/UiInteractable.h"

class MainMenuController : public GameObject {
  public:
    UiInteractable* playButton = nullptr;
    UiInteractable* optionsButton = nullptr;
    UiInteractable* backButton = nullptr;

    UiLayout* mainMenuLayout = nullptr;
    UiLayout* optionsMenuLayout = nullptr;

    MainMenuController() = default;

    void Update() {
        if (playButton && playButton->isPressed) {
            Application::Get()->RequestSceneChange(
                [](Scene* s) { TestScene::InitScene(*s); });
        }

        if (optionsButton && optionsButton->isPressed) {
            if (mainMenuLayout)
                mainMenuLayout->offset = glm::ivec2(9999, 9999);
            if (optionsMenuLayout)
                optionsMenuLayout->offset = glm::ivec2(0, 0);
        }

        if (backButton && backButton->isPressed) {
            if (optionsMenuLayout)
                optionsMenuLayout->offset = glm::ivec2(9999, 9999);
            if (mainMenuLayout)
                mainMenuLayout->offset = glm::ivec2(0, 0);
        }
    }
};

namespace MainMenu {
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

    SceneNode* logicNode = mainScene.CreateNode("MainMenu Logic");
    auto* controller = logicNode->AddObject<MainMenuController>();

    SceneNode* mainMenuGroup = mainScene.CreateNode("Main Menu Group");
    controller->mainMenuLayout = mainMenuGroup->AddObject<UiLayout>(
        glm::uvec2(400, 300), glm::ivec2(0, 0), 0, AnchorPoint::Center);

    SceneNode* playButtonNode =
        mainScene.CreateNode(mainMenuGroup, "Play Button");
    playButtonNode->AddObject<UiLayout>(glm::uvec2(200, 50), glm::ivec2(0, 50),
                                        1, AnchorPoint::Center);
    auto* playVisual =
        playButtonNode->AddObject<UiVisual>(glm::vec4(0.2f, 0.6f, 0.2f, 1.0f));
    playVisual->colorHovered = glm::vec4(0.3f, 0.8f, 0.3f, 1.0f);
    controller->playButton = playButtonNode->AddObject<UiInteractable>();

    SceneNode* playTextNode = mainScene.CreateNode(playButtonNode, "Play Text");
    playTextNode->AddObject<UiLayout>(glm::uvec2(200, 50), glm::ivec2(0, 0), 2,
                                      AnchorPoint::Center);
    auto* playText = playTextNode->AddObject<UiText>("Play Game", font);
    playText->fontSize = 24.0f;

    SceneNode* optionsButtonNode =
        mainScene.CreateNode(mainMenuGroup, "Options Button");
    optionsButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 50), glm::ivec2(0, -50), 1, AnchorPoint::Center);
    auto* optionsVisual = optionsButtonNode->AddObject<UiVisual>(
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    optionsVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->optionsButton = optionsButtonNode->AddObject<UiInteractable>();

    SceneNode* optionsTextNode =
        mainScene.CreateNode(optionsButtonNode, "Options Text");
    optionsTextNode->AddObject<UiLayout>(glm::uvec2(200, 50), glm::ivec2(0, 0),
                                         2, AnchorPoint::Center);
    auto* optionsText = optionsTextNode->AddObject<UiText>("Options", font);
    optionsText->fontSize = 24.0f;

    SceneNode* optionsGroup = mainScene.CreateNode("Options Menu Group");
    controller->optionsMenuLayout = optionsGroup->AddObject<UiLayout>(
        glm::uvec2(400, 300), glm::ivec2(9999, 9999), 0, AnchorPoint::Center);

    SceneNode* backButtonNode =
        mainScene.CreateNode(optionsGroup, "Back Button");
    backButtonNode->AddObject<UiLayout>(
        glm::uvec2(150, 40), glm::ivec2(0, -150), 6, AnchorPoint::BottomCenter);
    auto* backVisual =
        backButtonNode->AddObject<UiVisual>(glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
    backVisual->colorHovered = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
    controller->backButton = backButtonNode->AddObject<UiInteractable>();

    SceneNode* backTextNode = mainScene.CreateNode(backButtonNode, "Back Text");
    backTextNode->AddObject<UiLayout>(glm::uvec2(150, 40), glm::ivec2(0, 0), 7,
                                      AnchorPoint::Center);
    auto* backText = backTextNode->AddObject<UiText>("Back", font);
    backText->fontSize = 20.0f;
}
} // namespace MainMenu
