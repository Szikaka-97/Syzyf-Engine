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

    UiInteractable* resolutionButton = nullptr;
    UiInteractable* fullscreenButton = nullptr;
    UiInteractable* vsyncToggleButton = nullptr;
    UiInteractable* ssaoButton = nullptr;
    UiInteractable* applyButton = nullptr;

    UiLayout* mainMenuLayout = nullptr;
    UiLayout* optionsMenuLayout = nullptr;

    int pendingResolutionIndex = 0;
    int resWidths[4] = {1280, 1280, 800, 1920};
    int resHeights[4] = {720, 800, 600, 1080};

    bool pendingWindowed = true;
    bool pendingVsync = true;
    bool pendingSsao = true;

    bool initialized = false;

    MainMenuController() = default;

    void Update() {
        if (!initialized) {
            auto* app = Application::Get();
            if (app) {
                for (int i = 0; i < 4; ++i) {
                    if (resWidths[i] == app->GetSettings().resolutionWidth &&
                        resHeights[i] == app->GetSettings().resolutionHeight) {
                        pendingResolutionIndex = i;
                        break;
                    }
                }
                pendingWindowed = app->GetSettings().windowed;
                pendingVsync = app->GetSettings().vsyncEnabled;
                pendingSsao = app->GetSettings().ssaoEnabled;
            }
            initialized = true;
        }

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

        if (resolutionButton && resolutionButton->isPressed) {
            pendingResolutionIndex = (pendingResolutionIndex + 1) % 4;
            spdlog::debug("Resolution set to {}x{}",
                          resWidths[pendingResolutionIndex],
                          resHeights[pendingResolutionIndex]);
        }

        if (fullscreenButton && fullscreenButton->isPressed) {
            pendingWindowed = !pendingWindowed;
            spdlog::debug("Windowed toggled: {}", pendingWindowed);
        }

        if (vsyncToggleButton && vsyncToggleButton->isPressed) {
            pendingVsync = !pendingVsync;
            spdlog::debug("VSync toggled: {}", pendingVsync);
        }

        if (ssaoButton && ssaoButton->isPressed) {
            pendingSsao = !pendingSsao;
            spdlog::debug("SSAO toggled: {}", pendingSsao);
        }

        if (applyButton && applyButton->isPressed) {
            auto* app = Application::Get();
            if (app) {
                app->GetSettings().resolutionWidth =
                    resWidths[pendingResolutionIndex];
                app->GetSettings().resolutionHeight =
                    resHeights[pendingResolutionIndex];
                app->GetSettings().windowed = pendingWindowed;
                app->GetSettings().vsyncEnabled = pendingVsync;
                app->GetSettings().ssaoEnabled = pendingSsao;
                app->GetSettings().Save();
                app->ApplySettings();
            }

            spdlog::debug("Applying settings: Resolution={}x{}, Windowed={}, "
                          "VSync={}, SSAO={}",
                          app->GetSettings().resolutionWidth,
                          app->GetSettings().resolutionHeight,
                          app->GetSettings().windowed,
                          app->GetSettings().vsyncEnabled,
                          app->GetSettings().ssaoEnabled);
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
        glm::uvec2(400, 500), glm::ivec2(9999, 9999), 0, AnchorPoint::Center);

    SceneNode* resolutionButtonNode =
        mainScene.CreateNode(optionsGroup, "Resolution Button");
    resolutionButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 200), 1, AnchorPoint::Center);
    auto* resolutionVisual = resolutionButtonNode->AddObject<UiVisual>(
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    resolutionVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->resolutionButton =
        resolutionButtonNode->AddObject<UiInteractable>();

    SceneNode* resolutionTextNode =
        mainScene.CreateNode(resolutionButtonNode, "Resolution Text");
    resolutionTextNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 0), 2, AnchorPoint::Center);
    auto* resolutionText =
        resolutionTextNode->AddObject<UiText>("Cycle Resolution", font);
    resolutionText->fontSize = 20.0f;

    SceneNode* fullscreenButtonNode =
        mainScene.CreateNode(optionsGroup, "Fullscreen Button");
    fullscreenButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 150), 1, AnchorPoint::Center);
    auto* fullscreenVisual = fullscreenButtonNode->AddObject<UiVisual>(
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    fullscreenVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->fullscreenButton =
        fullscreenButtonNode->AddObject<UiInteractable>();

    SceneNode* fullscreenTextNode =
        mainScene.CreateNode(fullscreenButtonNode, "Fullscreen Text");
    fullscreenTextNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 0), 2, AnchorPoint::Center);
    auto* fullscreenText =
        fullscreenTextNode->AddObject<UiText>("Toggle Windowed", font);
    fullscreenText->fontSize = 20.0f;

    SceneNode* vsyncButtonNode =
        mainScene.CreateNode(optionsGroup, "VSync Button");
    vsyncButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 100), 1, AnchorPoint::Center);
    auto* vsyncVisual =
        vsyncButtonNode->AddObject<UiVisual>(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    vsyncVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->vsyncToggleButton =
        vsyncButtonNode->AddObject<UiInteractable>();

    SceneNode* vsyncTextNode =
        mainScene.CreateNode(vsyncButtonNode, "VSync Text");
    vsyncTextNode->AddObject<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 2,
                                       AnchorPoint::Center);
    auto* vsyncText = vsyncTextNode->AddObject<UiText>("Toggle VSync", font);
    vsyncText->fontSize = 20.0f;

    SceneNode* ssaoButtonNode =
        mainScene.CreateNode(optionsGroup, "SSAO Button");
    ssaoButtonNode->AddObject<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0),
                                        1, AnchorPoint::Center);
    auto* ssaoVisual =
        ssaoButtonNode->AddObject<UiVisual>(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    ssaoVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->ssaoButton = ssaoButtonNode->AddObject<UiInteractable>();

    SceneNode* ssaoTextNode = mainScene.CreateNode(ssaoButtonNode, "SSAO Text");
    ssaoTextNode->AddObject<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 2,
                                      AnchorPoint::Center);
    auto* ssaoText = ssaoTextNode->AddObject<UiText>("Toggle SSAO", font);
    ssaoText->fontSize = 20.0f;

    SceneNode* applyButtonNode =
        mainScene.CreateNode(optionsGroup, "Apply Button");
    applyButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, -50), 1, AnchorPoint::Center);
    auto* applyVisual =
        applyButtonNode->AddObject<UiVisual>(glm::vec4(0.2f, 0.6f, 0.2f, 1.0f));
    applyVisual->colorHovered = glm::vec4(0.3f, 0.8f, 0.3f, 1.0f);
    controller->applyButton = applyButtonNode->AddObject<UiInteractable>();

    SceneNode* applyTextNode =
        mainScene.CreateNode(applyButtonNode, "Apply Text");
    applyTextNode->AddObject<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 2,
                                       AnchorPoint::Center);
    auto* applyText = applyTextNode->AddObject<UiText>("Apply Changes", font);
    applyText->fontSize = 20.0f;

    SceneNode* backButtonNode =
        mainScene.CreateNode(optionsGroup, "Back Button");
    backButtonNode->AddObject<UiLayout>(glm::uvec2(150, 40), glm::ivec2(0, 50),
                                        6, AnchorPoint::BottomCenter);
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
