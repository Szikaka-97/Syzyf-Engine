#include "include/EditorApplication.h"
#include "CameraController.h"
#include "ComponentRegistry.h"
#include "MousePickingBodySystem.h"
#include "SceneRegistry.h"
#include "TestScene.h"
#include "Themes.h"

#include "thirdparty/ImGuizmo.h"
#include "thirdparty/ImViewGuizmo.h"

#include <Graphics.h>
#include <Scene.h>
#include <filesystem>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Editor {

void EditorApplication::InitSpdlog() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto imgui_sink = std::make_shared<ImGuiConsoleSink<std::mutex>>();
    imgui_sink->set_pattern("[%H:%M:%S] [%l] %v");

    std::vector<spdlog::sink_ptr> sinks{console_sink, imgui_sink};
    auto combined_logger =
        std::make_shared<spdlog::logger>("Syzyf", sinks.begin(), sinks.end());
    spdlog::set_default_logger(combined_logger);
}

void EditorApplication::OnInit(int argc, char* argv[]) {
    this->context.window = this->window;
    this->context.glContext = this->glContext;

    this->settings.Load();
    this->InitSpdlog();
    SceneRegistry::RegisterScenes();
    ComponentRegistry::RegisterComponents();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "imgui.ini";

    io.Fonts->AddFontFromFileTTF(
        "./res/editor/fonts/Open_Sans/static/OpenSans-Regular.ttf", 15.0f);
    io.Fonts->AddFontFromFileTTF(
        "./res/editor/fonts/Open_Sans/static/OpenSans-Bold.ttf", 15.0f);
    this->context.consoleFont = io.Fonts->AddFontFromFileTTF(
        "./res/editor/fonts/JetBrains_Mono/static/JetBrainsMono-Regular.ttf",
        14.0f);

    if (!std::filesystem::exists("imgui.ini") &&
        std::filesystem::exists("default_editor_layout.ini")) {
        ImGui::LoadIniSettingsFromDisk("default_editor_layout.ini");
    }

    Themes::SetTheme(this->settings.theme);

    if (this->settings.isMaximized) {
        SDL_MaximizeWindow(this->window);
    }

    this->context.physicsDebugRenderer =
        std::make_unique<Physics::DebugRenderer>();

    ShaderProgram* debugShader =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/physics_debug/physics_debug.vert")
            .WithPixelShader("./res/shaders/physics_debug/physics_debug.frag")
            .Link();
    this->context.physicsDebugRenderer->Init(debugShader);

    this->context.selectedScene = Scene::CreateStandaloneScene();
    this->context.loadedScenes.push_back(this->context.selectedScene);
    TestScene::InitScene(*this->context.selectedScene);

    for (auto* scene : this->context.loadedScenes) {
        scene->GetGraphics()->UpdateScreenResolution(
            glm::vec2(1024.0f, 576.0f));
        scene->AddComponent<MousePickingBodySystem>();
        SceneNode* cameraNode = scene->CreateNode();
        cameraNode->AddObject<CameraController>();
        cameraNode->GlobalTransform().Position() = {0.0f, 1.0f, 0.0f};

        if (scene == this->context.selectedScene) {
            this->context.mainCamera = cameraNode->GetObject<Camera>();
            this->context.mainCamera->SetAsMainCamera();
        }
    }
}

void EditorApplication::OnUpdate() { this->Input(); }

void EditorApplication::OnRender() {}

void EditorApplication::OnImGuiRender() {
    ImGuizmo::BeginFrame();
    ImViewGuizmo::BeginFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    this->DrawPanels();

    if (this->context.selectedScene != nullptr) {
        this->context.selectedScene->FlushQueues();
    }
}

void EditorApplication::OnShutdown() {
    SDL_WindowFlags flags = SDL_GetWindowFlags(this->window);
    this->settings.isMaximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;

    if (!this->settings.isMaximized) {
        int w, h;
        SDL_GetWindowSize(this->window, &w, &h);
        this->settings.windowWidth = w;
        this->settings.windowHeight = h;
    }

    this->settings.Save();
}

void EditorApplication::Input() {
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z,
                        ImGuiInputFlags_RouteGlobal)) {
        this->context.commandHistory.Undo();
    }
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z,
                        ImGuiInputFlags_RouteGlobal)) {
        this->context.commandHistory.Redo();
    }
}

void EditorApplication::DrawPanels() {
    this->mainMenuBar.Draw(this->context, this->isRunning, this->settings);
    this->graphPanel.Draw(this->context);
    this->systemsDebugPanel.Draw(this->context);
    this->commandHistoryPanel.Draw(this->context);
    this->inspectorPanel.Draw(this->context);
    this->filesPanel.Draw();
    this->consolePanel.Draw(this->context);
    this->sceneViewPanel.Draw(this->context);
}

void EditorApplication::ExecutePendingSceneChange() {
    if (!this->isSceneChangeRequested)
        return;

    Scene* oldScene = this->context.selectedScene;

    Scene* newScene = Scene::CreateStandaloneScene();

    if (this->pendingSceneInitFunc) {
        this->pendingSceneInitFunc(newScene);
    }

    newScene->AddComponent<MousePickingBodySystem>();
    newScene->GetGraphics()->UpdateScreenResolution(glm::vec2(1024.0f, 576.0f));

    auto it = std::find(this->context.loadedScenes.begin(),
                        this->context.loadedScenes.end(), oldScene);
    if (it != this->context.loadedScenes.end()) {
        *it = newScene;
    } else {
        this->context.loadedScenes.push_back(newScene);
    }

    this->context.selectedScene = newScene;
    this->context.selectedNode = nullptr;

    auto cameras = newScene->FindObjectsOfType<Camera>();
    if (!cameras.empty()) {
        this->context.mainCamera = cameras.front();
        this->context.mainCamera = nullptr;
    } else {
        this->context.mainCamera = nullptr;
        spdlog::warn("Editor: The new scene doesn't have a Camera");
    }

    this->currentScene = newScene;
    if (oldScene) {
        delete oldScene;
    }

    this->isSceneChangeRequested = false;
    this->pendingSceneInitFunc = nullptr;
}

} // namespace Editor
