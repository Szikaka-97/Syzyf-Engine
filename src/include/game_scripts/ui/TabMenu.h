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
#include <game_scripts/ui/ScrollingList.h>


class TabMenu : public GameObject {
private:
    UiInteractable* settingsButton = nullptr;
    UiInteractable* pauseButton = nullptr;

    UiOptionsMenu* optionsMenu = nullptr;
    ScrollingList* itemList = nullptr; 

    bool gamePaused = false;

public:
    TabMenu() = default;

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

        SceneNode* radialWheelNode = mainScene->GetOrCreateNode(uiRoot, "Wheel");
        radialWheelNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(800, 800), glm::uvec2(0, 0), 0, AnchorPoint::Center);

        auto* customVisual =
            radialWheelNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        customVisual->SetEnabled(false);
        customVisual->customMaterial = wheelUiMaterial;

        auto* radialWheel = radialWheelNode->AddObjectIfMissing<UiRadialWheel>();
        radialWheel->AddObjectIfMissing<WheelTag>();
        radialWheel->material.reset(wheelUiMaterial);
        radialWheel->SetItemModels({
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
            "./res/models/butelka.glb",
        });

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

        // Item list
        SceneNode* itemListNode = mainScene->GetOrCreateNode(uiRoot, "Item List");
        this->itemList = itemListNode->AddObjectIfMissing<ScrollingList>();

        Texture2D* placeholder = mainScene->Resources()->Get<Texture2D>("./res/textures/stone.jpg", Texture2D::ColorTextureRGB);

        std::vector<ScrollingListItemData> listItems = {
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
            {"Item 1", placeholder},
        };

        this->itemList->Initialize(font, listItems);
        itemListNode->AddObjectIfMissing<WheelTag>();
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
