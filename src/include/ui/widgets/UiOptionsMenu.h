#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/objects/UiInteractable.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/objects/UiVisual.h"
#include "ui/widgets/UiDropdown.h"
#include "ui/widgets/UiCheckbox.h"

#include <functional>
#include <spdlog/spdlog.h>

class UiOptionsMenu : public GameObject {
  public:
    UiDropdown* resolutionDropdown = nullptr;
    UiCheckbox* fullscreenCheckbox = nullptr;
    UiCheckbox* vsyncCheckbox = nullptr;
    UiInteractable* applyButton = nullptr;
    UiInteractable* backButton = nullptr;

    UiLayout* optionsMenuLayout = nullptr;

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

    controller->optionsMenuLayout = optionsGroup->AddObject<UiLayout>(
        glm::uvec2(400, 500), glm::ivec2(9999, 9999), 0, AnchorPoint::Center);

    // Resolution dropdown

    // This checks the currently set settings resolution, so the dropdown displays the proper value by default
    int initialResolutionIndex = 0;
    for (int i = 0; i < 4; ++i) {
        if (controller->resWidths[i] == settings.resolutionWidth &&
            controller->resHeights[i] == settings.resolutionHeight) {
            initialResolutionIndex = i;
            break;
        }
    }
    controller->pendingResolutionIndex = initialResolutionIndex;
    
    std::vector<std::string> resolutionOptions = {
        "1280x720",
        "1280x800",
        "800x600",
        "1920x1080"
    };

    SceneNode* dropdownNode = UiDropdown::Create(mainScene, font, resolutionOptions, controller->pendingResolutionIndex, optionsGroup);

    if (auto* layout = dropdownNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 200);
    }

    controller->resolutionDropdown = dropdownNode->GetObject<UiDropdown>();
    if (controller->resolutionDropdown) {
        controller->resolutionDropdown->OnValueChanged = [controller](int newIndex) {
            controller->pendingResolutionIndex = newIndex;
            spdlog::debug("Resolution dropdown changed to {}x{}", controller->resWidths[newIndex], controller->resHeights[newIndex]);
        };
    }

    // Fullscreen 
    SceneNode* fullscreenCheckboxNode = UiCheckbox::Create(mainScene, font, "Toggle Windowed", settings.windowed, optionsGroup);
    if (auto* layout = fullscreenCheckboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 150);
    }
    controller->fullscreenCheckbox = fullscreenCheckboxNode->GetObject<UiCheckbox>();
    if (controller->fullscreenCheckbox) {
        controller->fullscreenCheckbox->OnValueChanged = [controller](bool isChecked) {
            controller->pendingWindowed = isChecked;
            spdlog::debug("Windowed toggled: {}", controller->pendingWindowed);
        };
    }

    // VSync 
    SceneNode* vsyncCheckboxNode = UiCheckbox::Create(mainScene, font, "Toggle VSync", settings.vsyncEnabled, optionsGroup);
    if (auto* layout = vsyncCheckboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 100);
    }
    controller->vsyncCheckbox = vsyncCheckboxNode->GetObject<UiCheckbox>();
    if (controller->vsyncCheckbox) {
        controller->vsyncCheckbox->OnValueChanged = [controller](bool isChecked) {
            controller->pendingVsync = isChecked;
            spdlog::debug("VSync toggled: {}", controller->pendingVsync);
        };
    }

    // SSAO 
    SceneNode* ssaoCheckboxNode = UiCheckbox::Create(mainScene, font, "SSAO Checkbox", settings.ssaoEnabled, optionsGroup);
    if (auto* layout = ssaoCheckboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(0, 50);
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

    // Apply button
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

    // Back button
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

    return controller;
}
} // namespace OptionsMenu
