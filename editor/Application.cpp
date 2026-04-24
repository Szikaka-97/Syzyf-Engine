#include "include/Application.h"
#include "CameraController.h"
#include "ComponentRegistry.h"
#include "MousePickingBodySystem.h"
#include "Themes.h"
#include "panels/ConsolePanel.h"
#include "scenes/DungeonGeneratorScene.h"
#include "scenes/TestScene.h"

#include "scenes/DungeonGeneratorScene.h"
#include "scenes/TestScene.h"
#include "thirdparty/ImGuizmo.h"
#include "thirdparty/ImViewGuizmo.h"
#include <imgui.h>

#include <Jolt/RegisterTypes.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <filesystem>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl/imgui_impl_opengl3.h>
#include <imgui_impl/imgui_impl_sdl3.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <Engine.h>
#include <Graphics.h>
#include <physics/Jolt.h>
#include <physics/System.h>

namespace Editor {
bool Application::InitProgram() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("Failed to initialize SDL3: {}", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_VERSION_MINOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    context.window = SDL_CreateWindow(
        "Syzyf Editor", this->settings.windowWidth, this->settings.windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!context.window) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        return false;
    }

    if (this->settings.isMaximized) {
        SDL_MaximizeWindow(context.window);
    }

    context.glContext = SDL_GL_CreateContext(context.window);
    SDL_GL_MakeCurrent(context.window, context.glContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        spdlog::error("Failed to initialize GLAD");
        return false;
    }

    Engine::window = context.window;

    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    JPH::Trace = Physics::TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = Physics::AssertFailedImpl;
#endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    return true;
}

void Application::InitSpdlog() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    auto imgui_sink = std::make_shared<ImGuiConsoleSink<std::mutex>>();
    imgui_sink->set_pattern("[%H:%M:%S] [%l] %v");

    std::vector<spdlog::sink_ptr> sinks{console_sink, imgui_sink};

    auto combined_logger =
        std::make_shared<spdlog::logger>("Syzyf", sinks.begin(), sinks.end());

    spdlog::set_default_logger(combined_logger);
}

bool Application::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.IniFilename = "imgui.ini";

    io.Fonts->AddFontFromFileTTF(
        "./res/fonts/Open_Sans/static/OpenSans-Regular.ttf", 15.0f);
    io.Fonts->AddFontFromFileTTF(
        "./res/fonts/Open_Sans/static/OpenSans-Bold.ttf", 15.0f);

    ImFont* consoleFont = io.Fonts->AddFontFromFileTTF(
        "./res/fonts/JetBrains_Mono/static/JetBrainsMono-Regular.ttf", 14.0f);
    this->context.consoleFont = consoleFont;

    if (!std::filesystem::exists("imgui.ini")) {
        if (std::filesystem::exists("default_editor_layout.ini")) {
            ImGui::LoadIniSettingsFromDisk("default_editor_layout.ini");
        }
    }

    Themes::SetTheme(this->settings.theme);

    ImGui_ImplSDL3_InitForOpenGL(context.window, context.glContext);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    return true;
}

bool Application::Setup() {
    this->settings.Load();
    this->InitSpdlog();
    ComponentRegistry::RegisterComponents();

    return this->InitProgram() && this->InitImGui();
}

void Application::Terminate() {
    SDL_WindowFlags flags = SDL_GetWindowFlags(this->context.window);
    this->settings.isMaximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;

    if (!this->settings.isMaximized) {
        int w, h;
        SDL_GetWindowSize(this->context.window, &w, &h);
        this->settings.windowWidth = w;
        this->settings.windowHeight = h;
    }

    this->settings.Save();

    ImGui::DestroyPlatformWindows();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(this->context.glContext);
    SDL_DestroyWindow(this->context.window);
    SDL_Quit();
}

void Application::MainLoop() {
    // temporary
    this->context.selectedScene = Scene::CreateStandaloneScene();
    this->context.loadedScenes.push_back(this->context.selectedScene);
    TestScene::InitScene(*this->context.selectedScene);
    Scene* dungeonScene = Scene::CreateStandaloneScene();
    DungeonGeneratorScene::InitScene(*dungeonScene);
    this->context.loadedScenes.push_back(dungeonScene);

    for (auto* scene : this->context.loadedScenes) {
        scene->GetGraphics()->UpdateScreenResolution(
            glm::vec2(1024.0f, 576.0f)); // this doesnt do anything anymore ?
        scene->AddComponent<MousePickingBodySystem>();
        SceneNode* cameraNode = scene->CreateNode();
        cameraNode->AddObject<CameraController>();

        if (scene == this->context.selectedScene) {
            this->context.mainCamera = cameraNode->GetObject<Camera>();
            this->context.mainCamera->SetAsMainCamera();
        }
    }

    bool shouldClose = false;
    while (!shouldClose) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                shouldClose = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(context.window))
                shouldClose = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        this->Input();

        ImGuizmo::BeginFrame();
        ImViewGuizmo::BeginFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        this->DrawPanels(shouldClose);

        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(context.window);
    }
}

void Application::Input() {
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z,
                        ImGuiInputFlags_RouteGlobal)) {
        this->context.commandHistory.Undo();
    }
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z,
                        ImGuiInputFlags_RouteGlobal)) {
        this->context.commandHistory.Redo();
    }
}

void Application::DrawPanels(bool& shouldClose) {
    this->mainMenuBar.Draw(this->context, shouldClose, this->settings);
    this->graphPanel.Draw(this->context);
    this->systemsDebugPanel.Draw(this->context);
    this->inspectorPanel.Draw(this->context);
    this->filesPanel.Draw();
    this->consolePanel.Draw(this->context);
    this->sceneViewPanel.Draw(this->context);
    // this->statusBar.Draw(); a bit broken
}
} // namespace Editor
