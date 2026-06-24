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
    SceneNode* optionsGroup = mainScene.GetOrCreateNode("Options Menu Group");

    auto* app = Application::Get();
    if (app == nullptr) {
        spdlog::error("UiOptionsMenu: Failed to retrieve app");
        return nullptr;
    }
    auto& settings = app->GetSettings();

    auto* controller = optionsGroup->AddObjectIfMissing<UiOptionsMenu>();

    SceneNode* bgNode = mainScene.GetOrCreateNode(optionsGroup, "Settings Background");
    controller->bgLayout = bgNode->AddObjectIfMissing<UiLayout>(
        glm::uvec2(4000, 4000), glm::ivec2(9999, 9999), 90, AnchorPoint::Center);
    bgNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
    bgNode->AddObjectIfMissing<UiInteractable>();

    controller->optionsMenuLayout = optionsGroup->AddObjectIfMissing<UiLayout>(
        glm::uvec2(400, 500), glm::ivec2(9999, 9999), 100, AnchorPoint::Center);

    SceneNode* resolutionButtonNode =
        mainScene.GetOrCreateNode(optionsGroup, "Resolution Button");
    resolutionButtonNode->AddObjectIfMissing<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 200), 101, AnchorPoint::Center);
    auto* resolutionVisual = resolutionButtonNode->AddObjectIfMissing<UiVisual>(
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    resolutionVisual->colorHovered = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    controller->resolutionButton =
        resolutionButtonNode->AddObjectIfMissing<UiInteractable>();
    SceneNode* resolutionTextNode =
        mainScene.GetOrCreateNode(resolutionButtonNode, "Resolution Text");
    resolutionTextNode->AddObjectIfMissing<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, 0), 102, AnchorPoint::Center);
    auto* resolutionText =
        resolutionTextNode->AddObjectIfMissing<UiText>("Cycle Resolution", font);
    resolutionText->fontSize = 20.0f;

    SceneNode* vsyncCheckboxNode = UiCheckbox::Create(mainScene, font, 102, "VSync", settings.vsyncEnabled, optionsGroup);

    if (auto* layout = vsyncCheckboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 25);
        layout->zIndex = 101;
    }
    if (auto* checkboxLogic = vsyncCheckboxNode->GetObject<UiCheckbox>()) {
        checkboxLogic->OnValueChanged = [controller](bool isChecked) {
            if (isChecked) {
                spdlog::debug("VSync Checkbox ON");
                controller->pendingVsync = true;
            } else {
                spdlog::debug("VSync Checkbox OFF");
                controller->pendingVsync = false;
            }
        };
    }

    SceneNode* ssaoCheckboxNode = UiCheckbox::Create(mainScene, font, 102, "SSAO", settings.ssaoEnabled, optionsGroup);

    if (auto* layout = ssaoCheckboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 60);
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

    SceneNode* fullscreenCheckboxNode = UiCheckbox::Create(mainScene, font, 102, "Fullscreen", !settings.windowed, optionsGroup);

    if (auto* layout = fullscreenCheckboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 95);
        layout->zIndex = 101;
    }
    if (auto* checkboxLogic = fullscreenCheckboxNode->GetObject<UiCheckbox>()) {
        checkboxLogic->OnValueChanged = [controller](bool isChecked) {
            if (isChecked) {
                spdlog::debug("Fullscreen ON");
                controller->pendingWindowed = false;
            } else {
                spdlog::debug("Fullscreen OFF");
                controller->pendingWindowed = true;
            }
        };
    }

    SceneNode* applyButtonNode =
        mainScene.GetOrCreateNode(optionsGroup, "Apply Button");
    applyButtonNode->AddObjectIfMissing<UiLayout>(
        glm::uvec2(200, 40), glm::ivec2(0, -50), 101, AnchorPoint::Center);
    auto* applyVisual =
        applyButtonNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.2f, 0.6f, 0.2f, 1.0f));
    applyVisual->colorHovered = glm::vec4(0.3f, 0.8f, 0.3f, 1.0f);
    controller->applyButton = applyButtonNode->AddObjectIfMissing<UiInteractable>();
    SceneNode* applyTextNode =
        mainScene.GetOrCreateNode(applyButtonNode, "Apply Text");
    applyTextNode->AddObjectIfMissing<UiLayout>(glm::uvec2(200, 40), glm::ivec2(0, 0), 102,
                                       AnchorPoint::Center);
    auto* applyText = applyTextNode->AddObjectIfMissing<UiText>("Apply Changes", font);
    applyText->fontSize = 20.0f;

    SceneNode* backButtonNode =
        mainScene.GetOrCreateNode(optionsGroup, "Back Button");
    backButtonNode->AddObjectIfMissing<UiLayout>(glm::uvec2(150, 40), glm::ivec2(0, 50),
                                        101, AnchorPoint::BottomCenter);
    auto* backVisual =
        backButtonNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
    backVisual->colorHovered = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
    controller->backButton = backButtonNode->AddObjectIfMissing<UiInteractable>();
    SceneNode* backTextNode = mainScene.GetOrCreateNode(backButtonNode, "Back Text");
    backTextNode->AddObjectIfMissing<UiLayout>(glm::uvec2(150, 40), glm::ivec2(0, 0), 102,
                                      AnchorPoint::Center);
    auto* backText = backTextNode->AddObjectIfMissing<UiText>("Back", font);
    backText->fontSize = 20.0f;

    return controller;
}
} // namespace OptionsMenu
