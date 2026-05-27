#include "Application.h"
#include "physics/Jolt.h"

#include <Scene.h>
#include <TimeSystem.h>

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl/imgui_impl_sdl3.h>
#include <imgui_impl/imgui_impl_opengl3.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
extern "C" {
#ifdef __GNUC__
    __attribute__ ((dllexport)) unsigned long NvOptimusEnablement = 1;
    __attribute__ ((dllexport)) int AmdPowerXpressRequestHighPerformance = 1;
#else
    _declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif
}
#endif

static void APIENTRY glDebugOutput(
	GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char *message,
	const void *userParam
) {
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::string sourceString;

	switch (source) {
		case GL_DEBUG_SOURCE_API:             sourceString = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceString = "Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceString = "Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceString = "Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     sourceString = "Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           sourceString = "Other"; break;
	}

	std::string typeString;

	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               typeString = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeString = "Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeString = "Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         typeString = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         typeString = "Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              typeString = "Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          typeString = "Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           typeString = "Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               typeString = "Other"; break;
	}

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			if (source != GL_DEBUG_SOURCE_SHADER_COMPILER) { // Shader errors handled separately
				spdlog::error("GL {} {}: {} ({})", sourceString, typeString, message, id);
			
				//asm("INT3");

				throw 1;
			}

			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
		case GL_DEBUG_SEVERITY_LOW:          spdlog::warn("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: spdlog::info("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
	}
}

Application* Application::instance = nullptr;

Application::Application(const std::string& title, int width, int height)
    : windowTitle(title), windowWidth(width), windowHeight(height) {
        instance = this;
    }

Application::~Application() {}

bool Application::InitEngine() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | SDL_GL_CONTEXT_DEBUG_FLAG);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    this->window = SDL_CreateWindow(this->windowTitle.c_str(), windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    this->glContext = SDL_GL_CreateContext(this->window);
    SDL_GL_MakeCurrent(this->window, this->glContext);
    SDL_GL_SetSwapInterval(1);

    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    JPH::Trace = Physics::TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = Physics::AssertFailedImpl;
#endif

#ifndef NDEBUG
    int contextFlags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
    if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
    }
#endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplOpenGL3_Init("#version 460");
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);

    return true;
}

void Application::ShutdownEngine() {
    if (this->currentScene) {
        delete this->currentScene;
    }

    ImGui::DestroyPlatformWindows();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(this->glContext);
    SDL_DestroyWindow(this->window);
    SDL_Quit();
}

void Application::Run(int argc, char* argv[]) {
    if (!InitEngine()) return;

    OnInit(argc, argv);

    while (isRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) isRunning = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(this->window)) {
                isRunning = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        Time::Update();

        OnUpdate();

        int display_w, display_h;
        SDL_GetWindowSize(this->window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        OnRender();
        OnImGuiRender();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(this->window);

        if (this->isSceneChangeRequested) {
            ExecutePendingSceneChange();
        }
    }

    OnShutdown();
    ShutdownEngine();
}

SDL_Window* Application::GetWindow() {
    return instance ? instance->window : nullptr;
}

Scene* Application::GetCurrentScene() {
    return instance ? instance->currentScene : nullptr;
}

Application* Application::Get() {
    return instance;
}

void Application::RequestSceneBuild(SceneInitCallback initFunc) {
    this->pendingSceneInitFunc = initFunc;
    this->isSceneChangeRequested = true;
}

// Requests a scene change using a preloaded scene
void Application::RequestSceneChange(Scene* scene) {
    this->pendingScene = scene;
    this->isSceneChangeRequested = true;
}

// Swaps the scenes,
//  Will prioritize `pendingScene` over other pending scenes
//  Clears all the pending scenes afterwards regardless if there were more than one
void Application::ExecutePendingSceneChange() {
    if (!this->isSceneChangeRequested) return;

    if (this->currentScene) {
        delete this->currentScene;
        this->currentScene = nullptr;
    }

    // Preloaded scene
    if (this->pendingScene) {
        this->currentScene = this->pendingScene;
    // Init function
    } else if (this->pendingSceneInitFunc) {
        this->currentScene = Scene::CreateStandaloneScene();
        this->pendingSceneInitFunc(this->currentScene);
    }

    this->isSceneChangeRequested = false;
    this->pendingSceneInitFunc = nullptr;
    this->pendingScene = nullptr;

    this->ApplySettings();
}
