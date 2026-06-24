#pragma once

#include "CraftingScene.h"
#include "DungeonScene.h"
#include "Graphics.h"
#include "InputSystem.h"
#include "MainMenuScene.h"
#include "Serialization.h"
#include "TestScene.h"
#include "TutorialThrowingRoomScene.h"
#include "examples/fog_volume.h"
#include "examples/particles_and_scatter.h"
#include "examples/tweens.h"
#include "examples/ui.h"

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

class Scene;

namespace Editor {

using SceneInitFunction = std::function<void(Scene&)>;
using SceneLoadFunction = std::function<Scene*(void)>;

class SceneRegistry {
  public:
    static void RegisterScene(const std::string& name,
                              SceneInitFunction initFunction) {
        GetRegistry()[name] = std::move(initFunction);
    }

    static std::unordered_map<std::string, SceneInitFunction>& GetRegistry() {
        static std::unordered_map<std::string, SceneInitFunction> registry;
        return registry;
    }

    static std::unordered_map<std::string, SceneLoadFunction>&
    GetLoadRegistry() {
        static std::unordered_map<std::string, SceneLoadFunction> registry;
        return registry;
    }

    static void RegisterScenes() {
        SceneRegistry::RegisterScene("Test Scene", TestScene::InitScene);
        SceneRegistry::RegisterScene("Dungeon Generator",
                                     DungeonScene::InitScene);
        SceneRegistry::RegisterScene("Example: UI", ExampleUi::InitScene);
        SceneRegistry::RegisterScene("Example: Particles And Scatter",
                                     ExampleParticlesAndScatter::InitScene);
        SceneRegistry::RegisterScene("Example: Tweens",
                                     ExampleTweens::InitScene);
        SceneRegistry::RegisterScene("Example: Fog Volume",
                                     ExampleFogVolume::InitScene);
        SceneRegistry::RegisterScene("Main Menu", MainMenu::InitScene);
        // SceneRegistry::RegisterScene("Tutorial Throwing",
        //                              TutorialThrowingRoomScene::InitScene);
        SceneRegistry::RegisterScene("Crafting Scene",
                                     CraftingScene::InitScene);
        SceneRegistry::RegisterScene("Base Scene",
                                     BaseScene::InitScene);
        for (const auto& sceneFile :
             std::filesystem::directory_iterator("./res/scenes")) {
            GetLoadRegistry()[std::format("Loaded: {}",
                                          sceneFile.path().stem().string())] =
                [sceneFile]() -> Scene* {
                return Scene::LoadScene(sceneFile.path());
            };
        }
    }
};
} // namespace Editor
