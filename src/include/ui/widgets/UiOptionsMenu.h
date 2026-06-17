#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/objects/UiInteractable.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/objects/UiVisual.h"
#include "ui/widgets/UiCheckbox.h"

#include <functional>
#include <spdlog/spdlog.h>

class UiOptionsMenu : public GameObject {
  public:
    UiInteractable* resolutionButton = nullptr;
    UiInteractable* fullscreenButton = nullptr;
    UiInteractable* vsyncToggleButton = nullptr;
    UiInteractable* applyButton = nullptr;
    UiInteractable* backButton = nullptr;

    UiLayout* optionsMenuLayout = nullptr;
    UiLayout* bgLayout = nullptr;

    int pendingResolutionIndex = 0;
    int resWidths[4] = {1280, 1280, 800, 1920};
    int resHeights[4] = {720, 800, 600, 1080};

    bool pendingWindowed = true;
    bool pendingVsync = true;
    bool pendingSsao = true;
    bool initialized = false;

    std::function<void()> onBackClicked;

    UiOptionsMenu() = default;

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

        if (resolutionButton && resolutionButton->isDown) {
            pendingResolutionIndex = (pendingResolutionIndex + 1) % 4;
            spdlog::debug("Resolution set to {}x{}",
                          resWidths[pendingResolutionIndex],
                          resHeights[pendingResolutionIndex]);
        }

        if (fullscreenButton && fullscreenButton->isDown) {
            pendingWindowed = !pendingWindowed;
            spdlog::debug("Windowed toggled: {}", pendingWindowed);
        }

        if (vsyncToggleButton && vsyncToggleButton->isDown) {
            pendingVsync = !pendingVsync;
            spdlog::debug("VSync toggled: {}", pendingVsync);
        }

        if (applyButton && applyButton->isDown) {
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

        if (backButton && backButton->isDown) {
            if (onBackClicked) {
                spdlog::warn("calling on back clicked");
                onBackClicked();
            }
        }
    }

    void SetVisible(bool visible) {
        if (optionsMenuLayout) {
            spdlog::info("Set Visible");
            optionsMenuLayout->offset =
                visible ? glm::ivec2(0, 0) : glm::ivec2(9999, 9999);
            spdlog::info("Set offsets to: {}x{}, visible: {}", optionsMenuLayout->offset.x, optionsMenuLayout->offset.y, visible);
        }
        if (bgLayout) {
            bgLayout->offset = visible ? glm::ivec2(0, 0) : glm::ivec2(9999, 9999);
        }
    }

    bool IsVisible() const {
        if (optionsMenuLayout) {
            return optionsMenuLayout->offset == glm::ivec2(0, 0);
        }
        return false;
    }
};

namespace OptionsMenu {
inline UiOptionsMenu* Build(Scene& mainScene, Font* font) {
    SceneNode* optionsGroup = mainScene.CreateNode("Options Menu Group");

    auto* app = Application::Get();
    if (app == nullptr) {
        spdlog::error("UiOptionsMenu: Failed to retrieve app");
        return nullptr;
    }
    auto& settings = app->GetSettings();

    auto* controller = optionsGroup->AddObject<UiOptionsMenu>();

    SceneNode* bgNode = mainScene.CreateNode(optionsGroup, "Settings Background");
    controller->bgLayout = bgNode->AddObject<UiLayout>(
        glm::uvec2(4000, 4000), glm::ivec2(9999, 9999), 90, AnchorPoint::Center);
    bgNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
    bgNode->AddObject<UiInteractable>();

    controller->optionsMenuLayout = optionsGroup->AddObject<UiLayout>(
        glm::uvec2(400, 500), glm::ivec2(9999, 9999), 100, AnchorPoint::Center);

    SceneNode* resolutionButtonNode =
        mainScene.CreateNode(optionsGroup, "Resolution Button");
    resolutionButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 200), 101, AnchorPoint::Center);
    auto* resolutionVisual = resolutionButtonNode->AddObject<UiVisual>(
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    resolutionVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->resolutionButton =
        resolutionButtonNode->AddObject<UiInteractable>();
    SceneNode* resolutionTextNode =
        mainScene.CreateNode(resolutionButtonNode, "Resolution Text");
    resolutionTextNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 0), 102, AnchorPoint::Center);
    auto* resolutionText =
        resolutionTextNode->AddObject<UiText>("Cycle Resolution", font);
    resolutionText->fontSize = 20.0f;

    SceneNode* fullscreenButtonNode =
        mainScene.CreateNode(optionsGroup, "Fullscreen Button");
    fullscreenButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 150), 101, AnchorPoint::Center);
    auto* fullscreenVisual = fullscreenButtonNode->AddObject<UiVisual>(
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    fullscreenVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->fullscreenButton =
        fullscreenButtonNode->AddObject<UiInteractable>();
    SceneNode* fullscreenTextNode =
        mainScene.CreateNode(fullscreenButtonNode, "Fullscreen Text");
    fullscreenTextNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 0), 102, AnchorPoint::Center);
    auto* fullscreenText =
        fullscreenTextNode->AddObject<UiText>("Toggle Windowed", font);
    fullscreenText->fontSize = 20.0f;

    SceneNode* vsyncButtonNode =
        mainScene.CreateNode(optionsGroup, "VSync Button");
    vsyncButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 100), 101, AnchorPoint::Center);
    auto* vsyncVisual =
        vsyncButtonNode->AddObject<UiVisual>(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    vsyncVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->vsyncToggleButton =
        vsyncButtonNode->AddObject<UiInteractable>();
    SceneNode* vsyncTextNode =
        mainScene.CreateNode(vsyncButtonNode, "VSync Text");
    vsyncTextNode->AddObject<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 102,
                                       AnchorPoint::Center);
    auto* vsyncText = vsyncTextNode->AddObject<UiText>("Toggle VSync", font);
    vsyncText->fontSize = 20.0f;

    SceneNode* ssaoCheckboxNode = UiCheckbox::Create(mainScene, font, 102, "SSAO Checkbox", settings.ssaoEnabled, optionsGroup);

    if (auto* layout = ssaoCheckboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 50);
        layout->zIndex = 101;
    }
    if (auto* checkboxLogic = ssaoCheckboxNode->GetObject<UiCheckbox>()) {
        checkboxLogic->OnValueChanged = [controller](bool isChecked) {
            if (isChecked) {
                spdlog::debug("SSAO Checkbox ON");
                controller->pendingSsao = true;
            } else {
                spdlog::debug("SSAO Checkbox OFF");
                controller->pendingSsao = false;
            }
        };
    }

    SceneNode* applyButtonNode =
        mainScene.CreateNode(optionsGroup, "Apply Button");
    applyButtonNode->AddObject<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, -50), 101, AnchorPoint::Center);
    auto* applyVisual =
        applyButtonNode->AddObject<UiVisual>(glm::vec4(0.2f, 0.6f, 0.2f, 1.0f));
    applyVisual->colorHovered = glm::vec4(0.3f, 0.8f, 0.3f, 1.0f);
    controller->applyButton = applyButtonNode->AddObject<UiInteractable>();
    SceneNode* applyTextNode =
        mainScene.CreateNode(applyButtonNode, "Apply Text");
    applyTextNode->AddObject<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 102,
                                       AnchorPoint::Center);
    auto* applyText = applyTextNode->AddObject<UiText>("Apply Changes", font);
    applyText->fontSize = 20.0f;

    SceneNode* backButtonNode =
        mainScene.CreateNode(optionsGroup, "Back Button");
    backButtonNode->AddObject<UiLayout>(glm::uvec2(150, 40), glm::ivec2(0, 50),
                                        101, AnchorPoint::BottomCenter);
    auto* backVisual =
        backButtonNode->AddObject<UiVisual>(glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
    backVisual->colorHovered = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
    controller->backButton = backButtonNode->AddObject<UiInteractable>();
    SceneNode* backTextNode = mainScene.CreateNode(backButtonNode, "Back Text");
    backTextNode->AddObject<UiLayout>(glm::uvec2(150, 40), glm::ivec2(0, 0), 102,
                                      AnchorPoint::Center);
    auto* backText = backTextNode->AddObject<UiText>("Back", font);
    backText->fontSize = 20.0f;

    return controller;
}
} // namespace OptionsMenu
