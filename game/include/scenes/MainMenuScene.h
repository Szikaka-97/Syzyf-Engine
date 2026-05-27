#pragma once

#include "Application.h"
#include "GameObject.h"
#include "LoadingScene.h"
#include "ui/objects/UiInteractable.h"
#include "ui/widgets/UiOptionsMenu.h"

class MainMenuController : public GameObject {
  public:
    UiInteractable* playButton = nullptr;
    UiInteractable* optionsButton = nullptr;

    UiLayout* mainMenuLayout = nullptr;
    UiOptionsMenu* optionsController = nullptr;

    Scene* loadingScene = nullptr;

    MainMenuController() {
        this->loadingScene = Scene::CreateStandaloneScene();
        LoadingScene::InitScene(*loadingScene);
    }

    ~MainMenuController() {
        if (this->loadingScene != nullptr) {
            delete this->loadingScene;
        }
    }

    void Update() {
        if (playButton && playButton->isDown && this->loadingScene != nullptr) {
            Application::Get()->RequestSceneChange(this->loadingScene);
            this->loadingScene = nullptr;
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

    UiOptionsMenu* optionsUi = OptionsMenu::Build(mainScene, font);
    controller->optionsController = optionsUi;

    controller->optionsButton->OnDown = [controller, optionsUi]() {
        spdlog::info("optiosn button clicked");
        auto* optionsLayout = optionsUi->GetObject<UiLayout>();
        spdlog::info("OptionsUi offsets: {}x{}", optionsLayout->offset.x,
                     optionsLayout->offset.y);
        optionsUi->SetVisible(true);
        if (controller->mainMenuLayout) {
            controller->mainMenuLayout->offset = glm::ivec2(9999, 9999);
        }
    };

    optionsUi->onBackClicked = [controller, optionsUi]() {
        optionsUi->SetVisible(false);
        if (controller->mainMenuLayout) {
            controller->mainMenuLayout->offset = glm::ivec2(0, 0);
        }
    };
}
} // namespace MainMenu
