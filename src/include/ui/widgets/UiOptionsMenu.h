#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/objects/UiInteractable.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/objects/UiVisual.h"
#include "ui/widgets/UiCheckbox.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>

class UiOptionsMenu : public GameObject {
  public:
    UiInteractable* resolutionButton = nullptr;
    UiInteractable* fullscreenButton = nullptr;
    UiInteractable* vsyncToggleButton = nullptr;
    UiInteractable* soundVolumeDownButton = nullptr;
    UiInteractable* soundVolumeUpButton = nullptr;
    UiInteractable* ambientBrightnessDownButton = nullptr;
    UiInteractable* ambientBrightnessUpButton = nullptr;
    UiInteractable* backButton = nullptr;

    UiText* resolutionText = nullptr;
    UiText* windowModeText = nullptr;
    UiText* vsyncText = nullptr;
    UiText* soundVolumeText = nullptr;
    UiText* ambientBrightnessText = nullptr;

    UiLayout* optionsMenuLayout = nullptr;
    UiLayout* bgLayout = nullptr;

    int pendingResolutionIndex = 0;
    int resWidths[4] = {1280, 1280, 800, 1920};
    int resHeights[4] = {720, 800, 600, 1080};

    bool pendingWindowed = true;
    bool pendingVsync = true;
    bool pendingSsao = true;
    float pendingSoundVolume = 1.0f;
    float pendingAmbientBrightness = 1.0f;
    bool initialized = false;

    std::function<void()> onBackClicked;

    UiOptionsMenu() = default;

    void Update() {
        if (!initialized) {
            LoadCurrentSettings();
            initialized = true;
        }

        if (!IsVisible()) {
            return;
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

        if (soundVolumeDownButton && soundVolumeDownButton->isDown) {
            pendingSoundVolume = std::clamp(pendingSoundVolume - 0.1f, 0.0f, 1.0f);
        }

        if (soundVolumeUpButton && soundVolumeUpButton->isDown) {
            pendingSoundVolume = std::clamp(pendingSoundVolume + 0.1f, 0.0f, 1.0f);
        }

        if (ambientBrightnessDownButton && ambientBrightnessDownButton->isDown) {
            pendingAmbientBrightness = std::clamp(pendingAmbientBrightness - 0.1f, 0.1f, 3.0f);
        }

        if (ambientBrightnessUpButton && ambientBrightnessUpButton->isDown) {
            pendingAmbientBrightness = std::clamp(pendingAmbientBrightness + 0.1f, 0.1f, 3.0f);
        }

        UpdateValueTexts();

        if (backButton && backButton->isDown) {
            SavePendingSettings();
            if (onBackClicked) {
                spdlog::warn("calling on back clicked");
                onBackClicked();
            } else {
                SetVisible(false);
            }
        }
    }

    void SetVisible(bool visible) {
        if (visible) {
            LoadCurrentSettings();
            UpdateValueTexts();
        }

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

    void SavePendingSettings() {
        auto* app = Application::Get();
        if (app == nullptr) {
            return;
        }

        app->GetSettings().resolutionWidth = resWidths[pendingResolutionIndex];
        app->GetSettings().resolutionHeight = resHeights[pendingResolutionIndex];
        app->GetSettings().windowed = pendingWindowed;
        app->GetSettings().vsyncEnabled = pendingVsync;
        app->GetSettings().ssaoEnabled = pendingSsao;
        app->GetSettings().soundVolume = pendingSoundVolume;
        app->GetSettings().ambientBrightness = pendingAmbientBrightness;
        app->GetSettings().Save();
        app->ApplySettings();
    }

  private:
    void LoadCurrentSettings() {
        auto* app = Application::Get();
        if (app == nullptr) {
            return;
        }

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
        pendingSoundVolume = app->GetSettings().soundVolume;
        pendingAmbientBrightness = app->GetSettings().ambientBrightness;
    }

    void UpdateValueTexts() {
        if (resolutionText) {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "Resolution: %dx%d",
                          resWidths[pendingResolutionIndex],
                          resHeights[pendingResolutionIndex]);
            resolutionText->text = buffer;
        }

        if (windowModeText) {
            windowModeText->text = pendingWindowed ? "Window mode: Windowed" : "Window mode: Fullscreen";
        }

        if (vsyncText) {
            vsyncText->text = pendingVsync ? "VSync: On" : "VSync: Off";
        }

        if (soundVolumeText) {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "Sound volume: %d%%",
                          static_cast<int>(pendingSoundVolume * 100.0f + 0.5f));
            soundVolumeText->text = buffer;
        }

        if (ambientBrightnessText) {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "Game brightness: %d%%",
                          static_cast<int>(pendingAmbientBrightness * 100.0f + 0.5f));
            ambientBrightnessText->text = buffer;
        }
    }
};

namespace OptionsMenu {
inline UiLayout* ConfigureLayout(
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

inline UiVisual* ConfigureVisual(
    SceneNode* node,
    const glm::vec4& color,
    const glm::vec4& hoverColor = glm::vec4(-1.0f)
) {
    UiVisual* visual = node->AddObjectIfMissing<UiVisual>();
    visual->color = color;
    visual->colorHovered = hoverColor.x >= 0.0f ? std::optional<glm::vec4>(hoverColor) : std::nullopt;
    return visual;
}

inline UiInteractable* CreateButton(
    Scene& mainScene,
    SceneNode* parent,
    const std::string& name,
    const std::string& text,
    Font* font,
    const glm::uvec2& size,
    const glm::ivec2& offset,
    const glm::vec4& color = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
    const glm::vec4& hoverColor = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
    int zIndex = 121
) {
    SceneNode* buttonNode = mainScene.GetOrCreateNode(parent, name);
    ConfigureLayout(buttonNode, size, offset, zIndex, AnchorPoint::Center);
    ConfigureVisual(buttonNode, color, hoverColor);
    UiInteractable* interactable = buttonNode->AddObjectIfMissing<UiInteractable>();
    interactable->isInteractable = true;

    SceneNode* textNode = mainScene.GetOrCreateNode(buttonNode, name + " Text");
    ConfigureLayout(textNode, size, glm::ivec2(0, 0), zIndex + 1, AnchorPoint::Center);
    UiText* uiText = textNode->AddObjectIfMissing<UiText>();
    uiText->text = text;
    uiText->font = font;
    uiText->fontSize = 20.0f;
    uiText->alignment = TextAlignment::Middle;
    uiText->verticalAlignment = TextVerticalAlignment::Middle;

    return interactable;
}

inline UiText* CreateText(
    Scene& mainScene,
    SceneNode* parent,
    const std::string& name,
    const std::string& text,
    Font* font,
    const glm::uvec2& size,
    const glm::ivec2& offset,
    float fontSize = 20.0f,
    int zIndex = 122
) {
    SceneNode* textNode = mainScene.GetOrCreateNode(parent, name);
    ConfigureLayout(textNode, size, offset, zIndex, AnchorPoint::Center);
    UiText* uiText = textNode->AddObjectIfMissing<UiText>();
    uiText->text = text;
    uiText->font = font;
    uiText->fontSize = fontSize;
    uiText->alignment = TextAlignment::Middle;
    uiText->verticalAlignment = TextVerticalAlignment::Middle;
    return uiText;
}

inline UiOptionsMenu* Build(
    Scene& mainScene,
    Font* font,
    SceneNode* parent = nullptr,
    const std::string& groupName = "Options Menu Group"
) {
    SceneNode* optionsGroup = parent != nullptr
        ? mainScene.GetOrCreateNode(parent, groupName)
        : mainScene.GetOrCreateNode(groupName);

    auto* app = Application::Get();
    if (app == nullptr) {
        spdlog::error("UiOptionsMenu: Failed to retrieve app");
        return nullptr;
    }
    auto& settings = app->GetSettings();

    auto* controller = optionsGroup->AddObjectIfMissing<UiOptionsMenu>();

    controller->optionsMenuLayout = ConfigureLayout(
        optionsGroup, glm::uvec2(560, 660), glm::ivec2(9999, 9999), 120, AnchorPoint::Center);
    ConfigureVisual(optionsGroup, glm::vec4(0.08f, 0.08f, 0.08f, 0.98f));
    optionsGroup->AddObjectIfMissing<UiInteractable>();

    CreateText(
        mainScene, optionsGroup, "Options Title", "Options",
        font, glm::uvec2(440, 44), glm::ivec2(0, -280), 28.0f, 122);

    controller->resolutionText = CreateText(
        mainScene, optionsGroup, "Resolution Value Text", "Resolution: 1280x720",
        font, glm::uvec2(380, 36), glm::ivec2(0, -220), 18.0f, 122);
    controller->resolutionButton = CreateButton(
        mainScene, optionsGroup, "Resolution Button", "Change resolution",
        font, glm::uvec2(260, 40), glm::ivec2(0, -180));

    controller->windowModeText = CreateText(
        mainScene, optionsGroup, "Window Mode Value Text", "Window mode: Windowed",
        font, glm::uvec2(380, 36), glm::ivec2(0, -130), 18.0f, 122);
    controller->fullscreenButton = CreateButton(
        mainScene, optionsGroup, "Fullscreen Button", "Toggle window mode",
        font, glm::uvec2(260, 40), glm::ivec2(0, -90));

    controller->vsyncText = CreateText(
        mainScene, optionsGroup, "VSync Value Text", "VSync: On",
        font, glm::uvec2(380, 36), glm::ivec2(0, -40), 18.0f, 122);
    controller->vsyncToggleButton = CreateButton(
        mainScene, optionsGroup, "VSync Button", "Toggle VSync",
        font, glm::uvec2(260, 40), glm::ivec2(0, 0));

    SceneNode* ssaoCheckboxNode =
        UiCheckbox::Create(mainScene, font, 121, "SSAO", settings.ssaoEnabled, optionsGroup);

    if (auto* layout = ssaoCheckboxNode->GetObject<UiLayout>()) {
        layout->size = glm::ivec2(180, 40);
        layout->offset = glm::ivec2(0, 50);
        layout->zIndex = 121;
        layout->anchorPoint = AnchorPoint::Center;
    }
    if (auto* checkboxLogic = ssaoCheckboxNode->GetObject<UiCheckbox>()) {
        checkboxLogic->OnValueChanged = [controller](bool isChecked) {
            controller->pendingSsao = isChecked;
        };
    }

    controller->soundVolumeText = CreateText(
        mainScene, optionsGroup, "Sound Volume Value Text", "Sound volume: 100%",
        font, glm::uvec2(380, 36), glm::ivec2(0, 105), 18.0f, 122);
    controller->soundVolumeDownButton = CreateButton(
        mainScene, optionsGroup, "Sound Volume Down Button", "-",
        font, glm::uvec2(70, 38), glm::ivec2(-95, 145));
    controller->soundVolumeUpButton = CreateButton(
        mainScene, optionsGroup, "Sound Volume Up Button", "+",
        font, glm::uvec2(70, 38), glm::ivec2(95, 145));

    controller->ambientBrightnessText = CreateText(
        mainScene, optionsGroup, "Brightness Value Text", "Game brightness: 100%",
        font, glm::uvec2(380, 36), glm::ivec2(0, 195), 18.0f, 122);
    controller->ambientBrightnessDownButton = CreateButton(
        mainScene, optionsGroup, "Brightness Down Button", "-",
        font, glm::uvec2(70, 38), glm::ivec2(-95, 235));
    controller->ambientBrightnessUpButton = CreateButton(
        mainScene, optionsGroup, "Brightness Up Button", "+",
        font, glm::uvec2(70, 38), glm::ivec2(95, 235));

    controller->backButton = CreateButton(
        mainScene, optionsGroup, "Back Button", "Back",
        font, glm::uvec2(160, 42), glm::ivec2(0, 295),
        glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),
        glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));

    controller->SetVisible(false);
    return controller;
}
} // namespace OptionsMenu
