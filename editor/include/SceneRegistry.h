#pragma once

#include "Graphics.h"
#include "InputSystem.h"
#include "Serialization.h"
#include "scenes/DungeonGeneratorScene.h"
#include "scenes/LevelShowcaseScene.h"
#include "scenes/TestScene.h"
#include "scenes/examples/particles_and_scatter.h"
#include "scenes/examples/tweens.h"
#include "scenes/examples/ui.h"

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

    static std::unordered_map<std::string, SceneLoadFunction>& GetLoadRegistry() {
        static std::unordered_map<std::string, SceneLoadFunction> registry;
        return registry;
    }

    static void RegisterScenes() {
        SceneRegistry::RegisterScene("Test Scene", TestScene::InitScene);
        SceneRegistry::RegisterScene("Dungeon Generator",
                                     DungeonGeneratorScene::InitScene);
        SceneRegistry::RegisterScene("Level Showcase",
                                     LevelShowcaseScene::InitScene);
        SceneRegistry::RegisterScene("Example: UI", ExampleUi::InitScene);
        SceneRegistry::RegisterScene("Example: Particles And Scatter",
                                     ExampleParticlesAndScatter::InitScene);
        SceneRegistry::RegisterScene("Example: Tweens",
                                     ExampleTweens::InitScene);

        for (const auto& sceneFile : std::filesystem::directory_iterator("./res/scenes")) {
        	GetLoadRegistry()[std::format("Loaded: {}", sceneFile.path().stem().string())] = [sceneFile]() -> Scene * {
         		std::ifstream jsonFile{sceneFile.path()};

           		json sceneData = json::parse(jsonFile);
         
         		return Serialization::DeserializeObject<Scene>(sceneData);
         	};
        }
    }
};
} // namespace Editor
