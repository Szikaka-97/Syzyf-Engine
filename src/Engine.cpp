#include "SDL3/SDL_video.h"
#include "imgui.h"
#include "imgui_impl/imgui_impl_sdl3.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD

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

#include <Engine.h>

#include "physics/Jolt.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <Scene.h>
#include <TimeSystem.h>
#include <Graphics.h>

const char*   glsl_version     = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

SDL_Window* Engine::window = nullptr;
SDL_GLContextState* Engine::glContext = nullptr;
Scene* Engine::rootScene = nullptr;

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
	if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

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
		case GL_DEBUG_SEVERITY_HIGH:         spdlog::error("GL {} {}: {} ({})", sourceString, typeString, message, id); throw 1; break;
		case GL_DEBUG_SEVERITY_MEDIUM:
		case GL_DEBUG_SEVERITY_LOW:          spdlog::warn("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: spdlog::info("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
	}
}

bool Engine::InitProgram() {
	if (!SDL_Init(SDL_INIT_VIDEO))  {
		spdlog::error("Failed to initalize SDL!");

		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_VERSION_MAJOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_VERSION_MINOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_DisplayID mainScreen = SDL_GetPrimaryDisplay();
	const SDL_DisplayMode* mainScreenMode = SDL_GetDesktopDisplayMode(mainScreen);

	float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	window = SDL_CreateWindow("Syzyf Engine", (int) (mainScreenMode->w * mainScale), (int) (mainScreenMode->h * mainScale), windowFlags);
	if (window == NULL) {
		spdlog::error("Failed to create SDL Window!");

		return false;
	}

	glContext = SDL_GL_CreateContext(window);

	SDL_GL_MakeCurrent(window, glContext);
	SDL_GL_SetSwapInterval(1);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	bool err = !gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);

	if (err) {
		spdlog::error("Failed to initialize OpenGL loader!");

		return false;
	}

	// Jolt
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

	JPH::Trace = Physics::TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
	JPH::AssertFailed = Physics::AssertFailedImpl;
#endif

	int contextFlags = 0;
	glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);

	if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
	}
	else {
		spdlog::warn("Current machine does not support OpenGL debugging");
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	return true;
}

bool Engine::InitImGui() {
	if (!IMGUI_CHECKVERSION()) {
		return false;
	}

	ImGui::CreateContext();

	ImGui_ImplSDL3_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImGui::StyleColorsDark();

	return true;
}

void Engine::Terminate() {
	if (rootScene) {
		delete rootScene;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DestroyContext(glContext);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

void Engine::Update() {
	Time::Update();

	rootScene->Update();
}

void Engine::Render() {
	int display_w, display_h;
	SDL_GetWindowSize(window, &display_w, &display_h);

	rootScene->GetGraphics()->UpdateScreenResolution(glm::vec2(display_w, display_h));

	rootScene->Render();
}

void Engine::DrawImGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	static ImVec2 window_pos(0, 0);
	static float item_width = 230;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

	rootScene->DrawImGui();

	ImGui::End();

	ImGui::Render();
	int display_w, display_h;
	SDL_GetWindowSize(window, &display_w, &display_h);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Engine::Setup() {
	bool result = InitProgram() && InitImGui();

	if (result == false) {
		return false;
	}

	rootScene = Scene::CreateStandaloneScene();

	return true;
}

void Engine::MainLoop() {
	bool shouldClose = false;

	while (!shouldClose) {
		Update();

		Render();

		DrawImGui();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT) {
				shouldClose = true;
			}
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
				shouldClose = true;
			}
		}

		SDL_GL_SwapWindow(window);
	}
}

void Engine::Exit(int code) {
	Terminate();

	exit(code);
}

Scene* Engine::GetRoot() {
	return rootScene;
}

SDL_Window* Engine::GetWindow() {
	return window;
}
