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
#include <game_scripts/ui/UiMinimap.h>
#include <game_scripts/PotionInventory.h>


class TabMenu : public GameObject {
private:
    UiInteractable* settingsButton = nullptr;
    UiInteractable* pauseButton = nullptr;

    UiOptionsMenu* optionsMenu = nullptr;
    ScrollingList* itemList = nullptr;

    UiMinimap* minimap = nullptr;

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

        std::vector<fs::path> bottleModels;
        std::vector<int> potionSlotIndices;

        for (const auto& entry : PotionInventory::GetPotionInventory()) {
            if (bottleModels.size() >= 5) {
                break;
            }

            bottleModels.push_back(PotionInventory::PotionBottleModelPath(entry.data));
            potionSlotIndices.push_back(entry.slotIndex);
        }

        radialWheel->SetItemModels(bottleModels,potionSlotIndices);

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
            "./res/fonts/Papyrus/Papyrus-Regular.png", fontTextureParams);
            
        Font* font = mainScene->Resources()->Get<Font>(
            "./res/fonts/Papyrus/Papyrus-Regular.json", fontAtlas, true);

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

        this->itemList->Initialize(font);
        itemListNode->AddObjectIfMissing<WheelTag>();

        // Minimap
        SceneNode* minimapRootNode = mainScene->GetOrCreateNode(uiRoot, "Minimap");
        minimapRootNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(500, 500), glm::ivec2(40, 40), 10, AnchorPoint::TopLeft
        );

        this->minimap = minimapRootNode->AddObjectIfMissing<UiMinimap>();

        std::vector<DungeonGenerator*> generators = mainScene->FindObjectsOfType<DungeonGenerator>();
        if (!generators.empty()) {
            this->minimap->Initialize(generators[0]);
        }

        minimapRootNode->AddObjectIfMissing<WheelTag>();
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
