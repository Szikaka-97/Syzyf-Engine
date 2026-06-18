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
    Context* context = static_cast<Context*>(userdata);

    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Load Scene dialog error: {}", err);
            SDL_ClearError();
        }
        context->isNativeDialogOpen = false;
        return;
    }

    std::string filePath = filelist[0];

    context->DispatchToMainThread([context, filePath]() {
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

        context->isNativeDialogOpen = false;
    });
}

static void SDLCALL SaveSceneCallback(void* userdata,
                                      const char* const* filelist, int filter) {
    Context* context = static_cast<Context*>(userdata);

    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Save Scene dialog error: {}", err);
            SDL_ClearError();
        }
        context->isNativeDialogOpen = false;
        return;
    }

    std::string filePath = filelist[0];

    context->DispatchToMainThread([context, filePath]() {
        if (context->selectedScene) {
            try {
                nlohmann::json serializedScene =
                    Serialization::Serialize(context->selectedScene);

                fs::path savePath(filePath);
                if (!savePath.has_extension() ||
                    savePath.extension() != ".scene") {
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

        context->isNativeDialogOpen = false;
    });
}

void OpenLoadSceneDialog(Context& context) {
    const char* defaultLocation =
        lastSceneDirectory.empty() ? nullptr : lastSceneDirectory.c_str();

    context.isNativeDialogOpen = true;

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

    context.isNativeDialogOpen = true;

    SDL_ShowSaveFileDialog(SaveSceneCallback, &context, context.window,
                           sceneFilters, 2, defaultLocation.c_str());
}

// Prefab stuff
static const SDL_DialogFileFilter prefabFilters[] = {
    {"Syzyf Engine Prefab", "prefab"}, {"All Files", "*"}};

static std::string lastPrefabDirectory = "";

static void SDLCALL LoadPrefabCallback(void* userdata,
                                       const char* const* filelist,
                                       int filter) {
    Context* context = static_cast<Context*>(userdata);

    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Load Prefab dialog error: {}", err);
            SDL_ClearError();
        }
        context->isNativeDialogOpen = false;
        return;
    }

    std::string filePath = filelist[0];

    context->DispatchToMainThread([context, filePath]() {
        if (context->selectedScene) {
            try {
                fs::path path(filePath);
                lastPrefabDirectory = path.parent_path().string();

                SceneNode* parent = context->selectedNode
                                        ? context->selectedNode
                                        : context->selectedScene->GetRootNode();

                SceneNode* node = context->selectedScene->LoadPrefab(path);
                if (node) {
                    node->SetParent(parent);
                    spdlog::info("Loaded prefab from {}", filePath);
                }
            } catch (const std::exception& e) {
                spdlog::error("Failed to load prefab: {}", e.what());
            }
        }

        context->isNativeDialogOpen = false;
    });
}

struct SavePrefabPayload {
    Context* context;
    std::string data;
};

static void SDLCALL SavePrefabCallback(void* userdata,
                                       const char* const* filelist,
                                       int filter) {
    std::unique_ptr<SavePrefabPayload> payload(
        static_cast<SavePrefabPayload*>(userdata));
    Context* context = payload->context;

    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Save Prefab dialog error: {}", err);
            SDL_ClearError();
        }
        context->isNativeDialogOpen = false;
        return;
    }

    std::string filePath = filelist[0];
    std::string prefabData = std::move(payload->data);

    context->DispatchToMainThread([context, filePath, prefabData]() {
        try {
            fs::path savePath(filePath);
            if (!savePath.has_extension() ||
                savePath.extension() != ".prefab") {
                savePath.replace_extension(".prefab");
            }

            lastPrefabDirectory = savePath.parent_path().string();

            std::ofstream file(savePath);
            if (file.is_open()) {
                file << prefabData;
                spdlog::info("Saved prefab to {}", savePath.string());
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to save prefab: {}", e.what());
        }

        context->isNativeDialogOpen = false;
    });
}

void OpenLoadPrefabDialog(Context& context) {
    const char* defaultLocation =
        lastPrefabDirectory.empty() ? nullptr : lastPrefabDirectory.c_str();

    context.isNativeDialogOpen = true;

    SDL_ShowOpenFileDialog(LoadPrefabCallback, &context, context.window,
                           prefabFilters, 2, defaultLocation, false);
}

void OpenSavePrefabDialog(Context& context) {
    SceneNode* nodeToSave = context.selectedNode;

    if (!nodeToSave)
        return;

    nlohmann::json j = nodeToSave->SaveAsPrefab();

    SavePrefabPayload* payload = new SavePrefabPayload{&context, j.dump(4)};

    std::string defaultName = nodeToSave->GetName() + ".prefab";
    std::string defaultLocation =
        lastPrefabDirectory.empty()
            ? defaultName
            : (fs::path(lastPrefabDirectory) / defaultName).string();

    context.isNativeDialogOpen = true;

    SDL_ShowSaveFileDialog(SavePrefabCallback, payload, context.window,
                           prefabFilters, 2, defaultLocation.c_str());
}

static const SDL_DialogFileFilter textureFilters[] = {
    {"PNG Image", "png"}, {"HDR", "hdr"}, {"All Files", "*"}};

static std::string lastTextureDirectory = "";

struct SaveTexturePayload {
    Context* context;
    std::function<void(std::string)> saveFunc;
};

static void SDLCALL SaveTextureCallback(void* userdata,
                                        const char* const* filelist,
                                        int filter) {
    std::unique_ptr<SaveTexturePayload> payload(
        static_cast<SaveTexturePayload*>(userdata));

    Context* context = payload->context;

    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Save Texture dialog error: {}", err);
            SDL_ClearError();
        }
        context->isNativeDialogOpen = false;
        return;
    }

    std::string filePath = filelist[0];
    auto saveFunc = payload->saveFunc;

    context->DispatchToMainThread([context, filePath, saveFunc]() {
        try {
            fs::path savePath(filePath);

            if (!savePath.has_extension() || (savePath.extension() != ".png" &&
                                              savePath.extension() != ".hdr")) {
                savePath.replace_extension(".png");
            }

            lastTextureDirectory = savePath.parent_path().string();

            saveFunc(savePath.string());
        } catch (const std::exception& e) {
            spdlog::error("Failed to save texture: {}", e.what());
        }

        context->isNativeDialogOpen = false;
    });
}

void OpenSaveTextureDialog(Context& context,
                           std::function<void(std::string)> saveCallback) {
    std::string defaultName = "texture.png";
    std::string defaultLocation =
        lastTextureDirectory.empty()
            ? defaultName
            : (fs::path(lastTextureDirectory) / defaultName).string();

    SaveTexturePayload* payload =
        new SaveTexturePayload{&context, saveCallback};
    context.isNativeDialogOpen = true;

    SDL_ShowSaveFileDialog(SaveTextureCallback, payload, context.window,
                           textureFilters, 2, defaultLocation.c_str());
}

static const SDL_DialogFileFilter materialFilters[] = {
    {"Syzyf Material", "mat"}, {"All Files", "*"}};

static std::string lastMaterialDirectory = "";

struct MaterialDialogPayload {
    Context* context;
    std::function<void(std::string)> callback;
};

static void SDLCALL LoadMaterialCallback(void* userdata,
                                         const char* const* filelist,
                                         int filter) {
    std::unique_ptr<MaterialDialogPayload> payload(
        static_cast<MaterialDialogPayload*>(userdata));
    Context* context = payload->context;

    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Load Material dialog error: {}", err);
            SDL_ClearError();
        }
        context->isNativeDialogOpen = false;
        return;
    }

    std::string filePath = filelist[0];
    auto loadFunc = payload->callback;

    context->DispatchToMainThread([context, filePath, loadFunc]() {
        try {
            fs::path path(filePath);
            lastMaterialDirectory = path.parent_path().string();
            if (loadFunc)
                loadFunc(filePath);
        } catch (const std::exception& e) {
            spdlog::error("Failed to load material from dialog: {}", e.what());
        }
        context->isNativeDialogOpen = false;
    });
}

static void SDLCALL SaveMaterialCallback(void* userdata,
                                         const char* const* filelist,
                                         int filter) {
    std::unique_ptr<MaterialDialogPayload> payload(
        static_cast<MaterialDialogPayload*>(userdata));
    Context* context = payload->context;

    if (!filelist || !filelist[0]) {
        if (const char* err = SDL_GetError(); err && err[0] != '\0') {
            spdlog::error("Save Material dialog error: {}", err);
            SDL_ClearError();
        }
        context->isNativeDialogOpen = false;
        return;
    }

    std::string filePath = filelist[0];
    auto saveFunc = payload->callback;

    context->DispatchToMainThread([context, filePath, saveFunc]() {
        try {
            fs::path savePath(filePath);
            if (!savePath.has_extension() || savePath.extension() != ".mat") {
                savePath.replace_extension(".mat");
            }
            lastMaterialDirectory = savePath.parent_path().string();
            if (saveFunc)
                saveFunc(savePath.string());
        } catch (const std::exception& e) {
            spdlog::error("Failed to save material from dialog: {}", e.what());
        }
        context->isNativeDialogOpen = false;
    });
}

void OpenLoadMaterialDialog(Context& context,
                            std::function<void(std::string)> loadCallback) {
    const char* defaultLocation =
        lastMaterialDirectory.empty() ? nullptr : lastMaterialDirectory.c_str();

    MaterialDialogPayload* payload =
        new MaterialDialogPayload{&context, loadCallback};
    context.isNativeDialogOpen = true;

    SDL_ShowOpenFileDialog(LoadMaterialCallback, payload, context.window,
                           materialFilters, 2, defaultLocation, false);
}

void OpenSaveMaterialDialog(Context& context,
                            std::function<void(std::string)> saveCallback) {
    std::string defaultName = "NewMaterial.mat";
    std::string defaultLocation =
        lastMaterialDirectory.empty()
            ? defaultName
            : (fs::path(lastMaterialDirectory) / defaultName).string();

    MaterialDialogPayload* payload =
        new MaterialDialogPayload{&context, saveCallback};
    context.isNativeDialogOpen = true;

    SDL_ShowSaveFileDialog(SaveMaterialCallback, payload, context.window,
                           materialFilters, 2, defaultLocation.c_str());
}

} // namespace Editor
