#include "FileDialogHelpers.h"

#include "CameraController.h"
#include "EditorApplication.h"
#include "MousePickingBodySystem.h"

#include <Camera.h>
#include <Scene.h>

#include <SDL3/SDL_dialog.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Editor {

static const SDL_DialogFileFilter sceneFilters[] = {
    {"Syzyf Engine Scene", "scene"}, {"All Files", "*"}};

static std::string lastSceneDirectory = "";

static void SDLCALL LoadSceneCallback(void* userdata,
                                      const char* const* filelist, int filter) {
    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Load Scene dialog error: {}", err);
            SDL_ClearError();
        }
        return;
    }

    std::string filePath = filelist[0];
    Context* context = static_cast<Context*>(userdata);

    try {
        std::ifstream file(filePath);
        nlohmann::json data = nlohmann::json::parse(file);

        Scene* newScene = Serialization::DeserializeObject<Scene>(data);

        fs::path path(filePath);
        newScene->name = path.stem().string();
        lastSceneDirectory = path.parent_path().string();

        newScene->AddComponent<MousePickingBodySystem>();

        if (newScene->FindObjectsOfType<CameraController>().empty()) {
            SceneNode* cameraNode = newScene->CreateNode("Editor Camera");
            cameraNode->AddObject<CameraController>();
            cameraNode->AddObject<DoNotSerializeNode>();

            context->mainCamera = cameraNode->GetObject<Camera>();
        } else {
            context->mainCamera =
                newScene->FindObjectsOfType<CameraController>()[0]
                    ->GetObject<Camera>();
        }

        context->loadedScenes.push_back(newScene);
        context->selectedScene = newScene;
        context->selectedNode = nullptr;

        spdlog::info("Loaded scene from {}", filePath);
    } catch (const std::exception& e) {
        spdlog::error("Failed to load scene: {}", e.what());
    }
}

static void SDLCALL SaveSceneCallback(void* userdata,
                                      const char* const* filelist, int filter) {
    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Save Scene dialog error: {}", err);
            SDL_ClearError();
        }
        return;
    }

    std::string filePath = filelist[0];
    Context* context = static_cast<Context*>(userdata);

    if (context->selectedScene) {
        try {
            nlohmann::json serializedScene =
                Serialization::Serialize(context->selectedScene);

            fs::path savePath(filePath);
            if (!savePath.has_extension() || savePath.extension() != ".scene") {
                savePath.replace_extension(".scene");
            }

            lastSceneDirectory = savePath.parent_path().string();

            std::ofstream sceneSave(savePath);
            sceneSave << serializedScene.dump(2);

            context->selectedScene->name = savePath.stem().string();

            spdlog::info("Saved scene to {}", savePath.string());
        } catch (const std::exception& e) {
            spdlog::error("Failed to save scene: {}", e.what());
        }
    }
}

void OpenLoadSceneDialog(Context& context) {
    const char* defaultLocation =
        lastSceneDirectory.empty() ? nullptr : lastSceneDirectory.c_str();
    SDL_ShowOpenFileDialog(LoadSceneCallback, &context, context.window,
                           sceneFilters, 2, defaultLocation, false);
}

void OpenSaveSceneDialog(Context& context) {
    std::string defaultName = context.selectedScene
                                  ? (context.selectedScene->name + ".scene")
                                  : "New Scene.scene";

    std::string defaultLocation =
        lastSceneDirectory.empty()
            ? defaultName
            : (fs::path(lastSceneDirectory) / defaultName).string();

    SDL_ShowSaveFileDialog(SaveSceneCallback, &context, context.window,
                           sceneFilters, 2, defaultLocation.c_str());
}
} // namespace Editor
