#pragma once

#include "GameObject.h"
#include "game_scripts/PotionInventory.h"
#include "game_scripts/crafting/CraftingTypes.h"
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


class TabMenu : public GameObject {
private:
    UiInteractable* settingsButton = nullptr;
    UiInteractable* pauseButton = nullptr;

    UiOptionsMenu* optionsMenu = nullptr;
    ScrollingList* itemList = nullptr;

    UiMinimap* minimap = nullptr;

    bool gamePaused = false;

    UiRadialWheel* radialWheel = nullptr;
    std::vector<int> sliceToInventorySlotMap;

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

        this->radialWheel = radialWheelNode->AddObjectIfMissing<UiRadialWheel>();
        radialWheel->AddObjectIfMissing<WheelTag>();
        radialWheel->material.reset(wheelUiMaterial);

        this->RefreshPotionWheel();

        this->radialWheel->onSliceSelected = [this](int sliceIndex) {
            if (sliceIndex >= 0 && sliceIndex < this->sliceToInventorySlotMap.size()) {
                int realSlotIndex = this->sliceToInventorySlotMap[sliceIndex];

                PotionInventory::SetActivePotionSlot(realSlotIndex);
                spdlog::info("Equipped potion at slot {}", realSlotIndex);
            }
        };

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

    void RefreshPotionWheel() {
        if (!this->radialWheel) return;

        std::vector<PotionInventory::PotionInventoryEntry> potions = PotionInventory::GetPotionInventory();

        std::vector<fs::path> modelPaths;
        this->sliceToInventorySlotMap.clear();

        for (const auto& entry : potions) {
            fs::path modelPath = "./res/models/bottles/fire_bottle.glb";

            if (entry.data.primaryEffectId == Crafting::EffectId::Fire) {
                modelPath = "./res/models/bottles/fire_bottle.glb";
            } else if (entry.data.primaryEffectId == Crafting::EffectId::Explosion) {
                modelPath = "./res/models/bottles/explode_bottle.glb";
            } else if (entry.data.primaryEffectId == Crafting::EffectId::Confuse) {
                modelPath = "./res/models/bottles/confuse_bottle.glb";
            } else if (entry.data.primaryEffectId == Crafting::EffectId::Petrify) {
                modelPath = "./res/models/bottles/petrify_bottle.glb";
            } else if (entry.data.primaryEffectId == Crafting::EffectId::Tornado) {
                modelPath = "./res/models/bottles/tornado_bottle.glb";
            }

            modelPaths.push_back(modelPath);

            this->sliceToInventorySlotMap.push_back(entry.slotIndex);
        }

        this->radialWheel->SetItemModels(modelPaths);
        if (!modelPaths.empty()) {
            this->radialWheel->GetObject<UiVisual>()->SetEnabled(true);
        } else {
            this->radialWheel->GetObject<UiVisual>()->SetEnabled(false);
        }
    }

    void Update() {
        auto* wheelSystem = GetScene()->GetComponent<WheelSystem>();
        auto* input = GetScene()->Input();

        if (input->KeyDown(Key::Tab)) {
            this->RefreshPotionWheel();
        }

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
