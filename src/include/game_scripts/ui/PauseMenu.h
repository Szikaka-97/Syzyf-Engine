#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/widgets/UiOptionsMenu.h"
#include <Scene.h>
#include <InputSystem.h>
#include <TimeSystem.h>
#include <Resources.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>

class PauseMenu : public GameObject {
public:
    UiInteractable* optionsButton = nullptr;
    UiInteractable* exitButton = nullptr;

    UiLayout* pauseMenuLayout = nullptr;
    UiLayout* bgLayout = nullptr;

    UiOptionsMenu* optionsMenu = nullptr;

    bool isPaused = false;

    PauseMenu() = default;

    void Awake() {
        Scene* mainScene = GetScene();
        SceneNode* pauseGroup = GetNode();

        TextureParams fontTextureParams = {
            .channels = TextureChannels::RGB,
            .colorSpace = TextureColor::Linear,
            .format = TextureFormat::Ubyte,
            .wrapU = TextureWrap::Clamp,
            .wrapV = TextureWrap::Clamp,
            .minFilter = TextureFilter::Linear,
            .magFilter = TextureFilter::Linear
        };

        Texture2D* fontAtlas = mainScene->Resources()->Get<Texture2D>(
            "./res/fonts/OpenSans-Regular/OpenSans-Regular.png", fontTextureParams);
            
        Font* font = mainScene->Resources()->Get<Font>(
            "./res/fonts/OpenSans-Regular/OpenSans-Regular.json", fontAtlas);

        this->optionsMenu = OptionsMenu::Build(*mainScene, font);
        if (this->optionsMenu) {
            this->optionsMenu->SetVisible(false);
            this->optionsMenu->onBackClicked = [this]() {
                this->optionsMenu->SetVisible(false);
            };
        }

        SceneNode* bgNode = mainScene->GetOrCreateNode(pauseGroup, "Pause Background");
        this->bgLayout = bgNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(4000, 4000), glm::ivec2(9999, 9999), 80, AnchorPoint::Center);
        bgNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
        bgNode->AddObjectIfMissing<UiInteractable>();

        this->pauseMenuLayout = pauseGroup->AddObjectIfMissing<UiLayout>(
            glm::uvec2(400, 500), glm::ivec2(9999, 9999), 81, AnchorPoint::Center);

        SceneNode* optionsButtonNode = mainScene->GetOrCreateNode(pauseGroup, "Options Button");
        optionsButtonNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(200, 40), glm::ivec2(0, 50), 82, AnchorPoint::Center);
        auto* optionsVisual = optionsButtonNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
        optionsVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
        this->optionsButton = optionsButtonNode->AddObjectIfMissing<UiInteractable>();
        
        SceneNode* optionsTextNode = mainScene->GetOrCreateNode(optionsButtonNode, "Options Text");
        optionsTextNode->AddObjectIfMissing<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 83, AnchorPoint::Center);
        auto* optionsText = optionsTextNode->AddObjectIfMissing<UiText>("Options", font);
        optionsText->fontSize = 20.0f;

        SceneNode* exitButtonNode = mainScene->GetOrCreateNode(pauseGroup, "Exit Button");
        exitButtonNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(200, 40), glm::ivec2(0, -50), 82, AnchorPoint::Center);
        auto* exitVisual = exitButtonNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
        exitVisual->colorHovered = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
        this->exitButton = exitButtonNode->AddObjectIfMissing<UiInteractable>();
        
        SceneNode* exitTextNode = mainScene->GetOrCreateNode(exitButtonNode, "Exit Text");
        exitTextNode->AddObjectIfMissing<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 83, AnchorPoint::Center);
        auto* exitText = exitTextNode->AddObjectIfMissing<UiText>("Exit to Menu", font);
        exitText->fontSize = 20.0f;
    }

    void Update() {
        auto* input = GetScene()->GetComponent<InputSystem>();
        
        if (input && input->KeyDown(Key::Escape)) {
            if (optionsMenu && optionsMenu->IsVisible()) {
                optionsMenu->SetVisible(false);
            } else {
                SetPaused(!isPaused);
            }
        }

        if (isPaused && (!optionsMenu || !optionsMenu->IsVisible())) {
            if (optionsButton && optionsButton->isDown) {
                if (optionsMenu) {
                    optionsMenu->SetVisible(true);
                }
            }

            if (exitButton && exitButton->isDown) {
                Time::SetTimeScale(1.0f);
                Scene* menuScene = Scene::LoadScene("./res/scenes/Menu.scene");
                Application::Get()->RequestSceneChange(menuScene);
            }
        }
    }

    void SetPaused(bool paused) {
        isPaused = paused;
        Time::SetTimeScale(paused ? 0.0f : 1.0f);
        
        if (pauseMenuLayout) {
            pauseMenuLayout->offset = paused ? glm::ivec2(0, 0) : glm::ivec2(9999, 9999);
        }
        if (bgLayout) {
            bgLayout->offset = paused ? glm::ivec2(0, 0) : glm::ivec2(9999, 9999);
        }
    }
};
