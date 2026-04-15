#include "include/Application.h"
#include "InitScene.h"

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

    window = SDL_CreateWindow("Syzyf Editor", 1280, 720,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        spdlog::error("Failed to initialize GLAD");
        return false;
    }

    Engine::window = window;

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

bool Application::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.IniFilename = "imgui.ini";

    if (!std::filesystem::exists("imgui.ini")) {
        if (std::filesystem::exists("default_editor_layout.ini")) {
            ImGui::LoadIniSettingsFromDisk("default_editor_layout.ini");
        }
    }

    if (this->settings.darkThemeEnabled) {
        ImGui::StyleColorsDark();
    } else {
        ImGui::StyleColorsLight();
    }

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    return true;
}

bool Application::Setup() {
    this->settings.Load();

    return InitProgram() && InitImGui();
}

void Application::Terminate() {
    ImGui::DestroyPlatformWindows();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(this->glContext);
    SDL_DestroyWindow(this->window);
    SDL_Quit();
}

void Application::MainLoop() {
    // temporary
    this->context.selectedScene = Scene::CreateStandaloneScene();
    InitScene(*this->context.selectedScene, this->context.mainCamera);
    this->context.selectedScene->GetGraphics()->UpdateScreenResolution(
        glm::vec2(1024.0f, 576.0f));

    bool shouldClose = false;
    while (!shouldClose) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                shouldClose = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                shouldClose = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        ImViewGuizmo::BeginFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        this->mainMenuBar.Draw(shouldClose, this->settings);
        this->graphPanel.Draw(this->context);
        this->inspectorPanel.Draw(this->context);
        this->filesPanel.Draw();
        this->sceneViewPanel.Draw(this->context);

        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}
} // namespace Editor
