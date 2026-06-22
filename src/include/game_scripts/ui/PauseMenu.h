#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/widgets/UiOptionsMenu.h"

#include <InputSystem.h>
#include <Resources.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <optional>
#include <string>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>

class PauseMenu : public GameObject {
public:
    UiInteractable* resumeButton = nullptr;
    UiInteractable* optionsButton = nullptr;
    UiInteractable* mainMenuButton = nullptr;
    UiInteractable* desktopButton = nullptr;

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

        SceneNode* bgNode = mainScene->GetOrCreateNode(pauseGroup, "Pause Background");
        this->bgLayout = ConfigureLayout(
            bgNode, glm::uvec2(4000, 4000), glm::ivec2(9999, 9999), 80, AnchorPoint::Center);
        ConfigureVisual(bgNode, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
        bgNode->AddObjectIfMissing<UiInteractable>();

        this->pauseMenuLayout = ConfigureLayout(
            pauseGroup, glm::uvec2(420, 340), glm::ivec2(9999, 9999), 81, AnchorPoint::Center);
        ConfigureVisual(pauseGroup, glm::vec4(0.08f, 0.08f, 0.08f, 0.95f));
        pauseGroup->AddObjectIfMissing<UiInteractable>();

        this->resumeButton = CreatePauseButton(
            mainScene, pauseGroup, "Resume Button", "Resume", font, glm::ivec2(0, -105));

        this->optionsButton = CreatePauseButton(
            mainScene, pauseGroup, "Options Button", "Options", font, glm::ivec2(0, -35));

        this->mainMenuButton = CreatePauseButton(
            mainScene, pauseGroup, "Main Menu Button", "Main menu", font, glm::ivec2(0, 35));

        this->desktopButton = CreatePauseButton(
            mainScene, pauseGroup, "Desktop Button", "Desktop", font, glm::ivec2(0, 105),
            glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),
            glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));

        this->optionsMenu = OptionsMenu::Build(*mainScene, font, pauseGroup, "Pause Options Menu Group");
        if (this->optionsMenu) {
            this->optionsMenu->SetVisible(false);
            this->optionsMenu->onBackClicked = [this]() {
                this->optionsMenu->SetVisible(false);
            };
        }
    }

    void Update() {
        auto* input = GetScene()->GetComponent<InputSystem>();

        if (input && input->KeyDown(Key::Escape)) {
            if (optionsMenu && optionsMenu->IsVisible()) {
                optionsMenu->SavePendingSettings();
                optionsMenu->SetVisible(false);
            } else {
                SetPaused(!isPaused);
            }
        }

        if (!isPaused) {
            return;
        }

        if (optionsMenu && optionsMenu->IsVisible()) {
            return;
        }

        if (resumeButton && resumeButton->isDown) {
            SetPaused(false);
        }

        if (optionsButton && optionsButton->isDown) {
            if (optionsMenu) {
                optionsMenu->SetVisible(true);
            }
        }

        if (mainMenuButton && mainMenuButton->isDown) {
            Time::SetTimeScale(1.0f);
            Scene* mainMenu = Application::Get()->CreateStartingScreenScene();
            if (mainMenu != nullptr) {
                Application::Get()->RequestSceneChange(mainMenu);
            }
        }

        if (desktopButton && desktopButton->isDown) {
            Time::SetTimeScale(1.0f);
            Application::Get()->RequestQuit();
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
        if (!paused && optionsMenu) {
            optionsMenu->SetVisible(false);
        }
    }

private:
    UiLayout* ConfigureLayout(
        SceneNode* node,
        const glm::uvec2& size,
        const glm::ivec2& offset,
        int zIndex,
        AnchorPoint anchorPoint
    ) {
        UiLayout* layout = node->AddObjectIfMissing<UiLayout>();
        layout->size = glm::ivec2(size);
        layout->offset = offset;
        layout->zIndex = zIndex;
        layout->anchorPoint = anchorPoint;
        return layout;
    }

    UiVisual* ConfigureVisual(
        SceneNode* node,
        const glm::vec4& color,
        const glm::vec4& hoverColor = glm::vec4(-1.0f)
    ) {
        UiVisual* visual = node->AddObjectIfMissing<UiVisual>();
        visual->color = color;
        visual->colorHovered = hoverColor.x >= 0.0f ? std::optional<glm::vec4>(hoverColor) : std::nullopt;
        return visual;
    }

    UiText* CreateText(
        Scene* mainScene,
        SceneNode* parent,
        const std::string& name,
        const std::string& label,
        Font* font,
        const glm::uvec2& size,
        const glm::ivec2& offset,
        float fontSize,
        int zIndex
    ) {
        SceneNode* textNode = mainScene->GetOrCreateNode(parent, name);
        ConfigureLayout(textNode, size, offset, zIndex, AnchorPoint::Center);
        UiText* text = textNode->AddObjectIfMissing<UiText>();
        text->text = label;
        text->font = font;
        text->fontSize = fontSize;
        text->alignment = TextAlignment::Middle;
        text->verticalAlignment = TextVerticalAlignment::Middle;
        return text;
    }

    UiInteractable* CreatePauseButton(
        Scene* mainScene,
        SceneNode* parent,
        const std::string& name,
        const std::string& label,
        Font* font,
        const glm::ivec2& offset,
        const glm::vec4& color = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
        const glm::vec4& hoverColor = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)
    ) {
        SceneNode* buttonNode = mainScene->GetOrCreateNode(parent, name);
        ConfigureLayout(buttonNode, glm::uvec2(260, 46), offset, 82, AnchorPoint::Center);
        ConfigureVisual(buttonNode, color, hoverColor);
        UiInteractable* interactable = buttonNode->AddObjectIfMissing<UiInteractable>();
        interactable->isInteractable = true;

        CreateText(
            mainScene, buttonNode, name + " Text", label,
            font, glm::uvec2(260, 46), glm::ivec2(0, 0), 20.0f, 83);

        return interactable;
    }
};
