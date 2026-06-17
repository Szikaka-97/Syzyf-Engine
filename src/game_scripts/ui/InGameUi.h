#pragma once

#include "GameObject.h"
#include <Scene.h>
#include <Resources.h>
#include <TimeSystem.h>
#include <ui/widgets/UiOptionsMenu.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiVisual.h>
#include <ui/widgets/wheel/UiRadialWheel.h>
#include <ui/widgets/wheel/UiWheel.h>

class InGameUi : public GameObject {
private:
    UiInteractable* settingsButton = nullptr;
    UiInteractable* pauseButton = nullptr;

    UiOptionsMenu* optionsMenu = nullptr;

    bool gamePaused = false;

public:
    InGameUi() = default;

    void Awake() {
        Scene* mainScene = GetScene();
        SceneNode* uiRoot = GetNode();
        
        // Wheel
        ShaderProgram* wheelProgram =
            ShaderProgram::Build()
                .WithVertexShader("./res/shaders/ui/ui.vert")
                .WithPixelShader("./res/shaders/ui/custom/radial_wheel.frag")
                .Link();
        Material* wheelUiMaterial = new Material(wheelProgram);

        SceneNode* radialWheelNode = mainScene->CreateNode(uiRoot, "Wheel");
        radialWheelNode->AddObject<UiLayout>(
            glm::uvec2(600, 600), glm::uvec2(-150, 0), 0, AnchorPoint::CenterRight);

        auto* customVisual =
            radialWheelNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        customVisual->SetEnabled(false);
        customVisual->customMaterial = wheelUiMaterial;

        auto* radialWheel = radialWheelNode->AddObject<UiRadialWheel>();
        radialWheel->AddObject<WheelTag>();
        radialWheel->material.reset(wheelUiMaterial);
        radialWheel->SetItemModels({
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
        });

        // Right menu buttons
        SceneNode* rightMenuNode = mainScene->CreateNode(uiRoot, "Right Menu UI");
        rightMenuNode->AddObject<UiLayout>(
            // ------------------------------ hmm
            glm::uvec2(164, 640), glm::uvec2(-40, 40), 0, AnchorPoint::TopRight);
            
        //  Settings
        Texture2D* settingsIcon = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/settings_icon.png", Texture2D::ColorTextureRGBA);

        SceneNode* settingsButtonNode =
            mainScene->CreateNode(rightMenuNode, "Settings Button");
        settingsButtonNode->AddObject<UiLayout>(
            glm::uvec2(164, 142), glm::uvec2(0, -240), 1, AnchorPoint::Center);

        auto* settingsButtonVisual = settingsButtonNode->AddObject<UiVisual>(
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), settingsIcon);

        settingsButtonVisual->colorHovered = {0.8f, 0.8f, 0.8f, 1.0f};
        settingsButtonVisual->colorClicked = {0.4f, 0.4f, 0.4f, 1.0f};

        this->settingsButton = 
            settingsButtonNode->AddObject<UiInteractable>();
        settingsButtonNode->AddObject<WheelTag>();
        // Required so it's properly hidden on startup and works with the wheel
        settingsButtonVisual->SetEnabled(false);

        // Backpack
        Texture2D* backpackIcon = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/backpack_icon.png", Texture2D::ColorTextureRGBA);
        SceneNode* backpackButtonNode =
            mainScene->CreateNode(rightMenuNode, "Backpack Button");
        backpackButtonNode->AddObject<UiLayout>(
            glm::uvec2(164, 142), glm::uvec2(0, -80), 1, AnchorPoint::Center);
        auto* backpackButtonVisual = backpackButtonNode->AddObject<UiVisual>(
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), backpackIcon);
        backpackButtonVisual->colorHovered = {0.8f, 0.8f, 0.8f, 1.0f};
        backpackButtonVisual->colorClicked = {0.4f, 0.4f, 0.4f, 1.0f};
        auto* backpackButtonInteractable =
            backpackButtonNode->AddObject<UiInteractable>();
        backpackButtonNode->AddObject<WheelTag>();
        backpackButtonVisual->SetEnabled(false);

        // Pause
        Texture2D* pauseIcon = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/pause_icon.png", Texture2D::ColorTextureRGBA);
        SceneNode* pauseButtonNode =
            mainScene->CreateNode(rightMenuNode, "Pause Button");
        pauseButtonNode->AddObject<UiLayout>(
            glm::uvec2(164, 142), glm::uvec2(0, 80), 1, AnchorPoint::Center);
        auto* pauseButtonVisual = pauseButtonNode->AddObject<UiVisual>(
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), pauseIcon);
        pauseButtonVisual->colorHovered = {0.8f, 0.8f, 0.8f, 1.0f};
        pauseButtonVisual->colorClicked = {0.4f, 0.4f, 0.4f, 1.0f};
        this->pauseButton =
            pauseButtonNode->AddObject<UiInteractable>();
        pauseButtonNode->AddObject<WheelTag>();
        pauseButtonVisual->SetEnabled(false);

        // Map
        Texture2D* mapIcon = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/map_icon.png", Texture2D::ColorTextureRGBA);
        SceneNode* mapButtonNode =
            mainScene->CreateNode(rightMenuNode, "Map Button");
        mapButtonNode->AddObject<UiLayout>(glm::uvec2(164, 142), glm::uvec2(0, 240),
                                           1, AnchorPoint::Center);
        auto* mapButtonVisual = mapButtonNode->AddObject<UiVisual>(
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), mapIcon);
        mapButtonVisual->colorHovered = {0.8f, 0.8f, 0.8f, 1.0f};
        mapButtonVisual->colorClicked = {0.4f, 0.4f, 0.4f, 1.0f};
        auto* mapButtonInteractable = mapButtonNode->AddObject<UiInteractable>();
        mapButtonNode->AddObject<WheelTag>();
        mapButtonVisual->SetEnabled(false);

        // Potion Slots
        SceneNode* potionSlotsNode =
            mainScene->CreateNode(uiRoot, "Potion Slots UI");
        potionSlotsNode->AddObject<UiLayout>(glm::uvec2(899, 242),
                                             glm::uvec2(-40, -40), 0,
                                             AnchorPoint::BottomRight);
        Texture2D* potionSlotsBackgroundTexture =
            mainScene->Resources()->Get<Texture2D>(
                "./res/textures/ui/2d/potions_background.png",
                Texture2D::ColorTextureRGBA);
        potionSlotsNode
            ->AddObject<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                                  potionSlotsBackgroundTexture)
            ->SetEnabled(false);
        potionSlotsNode->AddObject<WheelTag>();

        // Health UI
        SceneNode* healthNode = mainScene->CreateNode(uiRoot, "Health UI");
        healthNode->AddObject<UiLayout>(glm::uvec2(826, 242), glm::uvec2(40, -40),
                                        0, AnchorPoint::BottomLeft);
        Texture2D* healthBackgroundTexture = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/life_background.png",
            Texture2D::ColorTextureRGBA);
        healthNode
            ->AddObject<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                                  healthBackgroundTexture)
            ->SetEnabled(false);
        healthNode->AddObject<WheelTag>();

        SceneNode* healthVialNode = mainScene->CreateNode(healthNode, "Health Vial");
        healthVialNode->AddObject<UiLayout>(glm::uvec2(826, 242), glm::uvec2(0, 0), 1, AnchorPoint::Center);
        healthVialNode->AddObject<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/life_foreground.png", Texture2D::ColorTextureRGBA))->SetEnabled(false);
        healthVialNode->AddObject<WheelTag>();

        TextureParams fontTextureParams = {
            .channels = TextureChannels::RGB,
            .colorSpace = TextureColor::Linear,
            .format = TextureFormat::Ubyte,
            .wrapU = TextureWrap::Clamp,
            .wrapV = TextureWrap::Clamp,
            .minFilter = TextureFilter::Linear,
            .magFilter = TextureFilter::Linear,
        };
        
        Texture2D* fontAtlas = mainScene->Resources()->Get<Texture2D>(
            "./res/fonts/OpenSans-Regular/OpenSans-Regular.png", fontTextureParams);
            
        Font* font = mainScene->Resources()->Get<Font>(
            "./res/fonts/OpenSans-Regular/OpenSans-Regular.json", fontAtlas);

        this->optionsMenu = OptionsMenu::Build(*mainScene, font);
        
        if (this->optionsMenu) {
            this->optionsMenu->SetVisible(false);
            this->optionsMenu->onBackClicked = [this, mainScene]() {
                this->optionsMenu->SetVisible(false);
                if (auto* ws = mainScene->GetComponent<WheelSystem>()) {
                    ws->isMenuBlocking = false;
                    if (!ws->isTabHeld) {
                        ws->CloseWheel();
                    } else {
                        Time::SetTimeScale(0.1f);
                    }
                } else {
                    Time::SetTimeScale(1.0f);
                }
            };
        }
    }

    void Update() {
        auto* wheelSystem = GetScene()->GetComponent<WheelSystem>();

        if (this->settingsButton != nullptr && settingsButton->isDown && this->optionsMenu != nullptr && !this->optionsMenu->IsVisible()) {
            optionsMenu->SetVisible(true);
            Time::SetTimeScale(0.0f);
           
            if (wheelSystem != nullptr) {
                wheelSystem->isMenuBlocking = true;
            }
        }

        if (this->pauseButton != nullptr && this->pauseButton->isDown) {
            this->gamePaused = !this->gamePaused;

            if (this->gamePaused) {
                Time::SetTimeScale(0.0f);
                if (wheelSystem) {
                    wheelSystem->isMenuBlocking = true;
                }
            } else {
                if (wheelSystem) {
                    wheelSystem->isMenuBlocking = false;
                    if (wheelSystem->isTabHeld) {
                        Time::SetTimeScale(0.1f);
                    } else {
                        Time::SetTimeScale(1.0f);
                        wheelSystem->CloseWheel();
                    }
                } else {
                    Time::SetTimeScale(1.0f);
                }
            }
        }
    }
};
