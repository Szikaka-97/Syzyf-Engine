#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#ifdef _WIN32
extern "C" {
#ifdef __GNUC__
    __attribute__ ((dllexport)) unsigned long NvOptimusEnablement = 1;
    __attribute__ ((dllexport)) int AmdPowerXpressRequestHighPerformance = 1;
#else
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif
}
#endif

#include <Engine.h>

#include "physics/PhysicsJolt.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Scene.h>
#include <TimeSystem.h>
#include <Graphics.h>

const char*   glsl_version     = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

GLFWwindow* Engine::window = nullptr;
Scene* Engine::rootScene = nullptr;


static void GLFWErrorCallback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

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
		case GL_DEBUG_SEVERITY_HIGH:         spdlog::error("GL {} {}: {} ({})", sourceString, typeString, message, id); exit(1); break;
		case GL_DEBUG_SEVERITY_MEDIUM:
		case GL_DEBUG_SEVERITY_LOW:          spdlog::warn("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: spdlog::info("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
	}
}

bool Engine::InitProgram() {
	glfwSetErrorCallback(GLFWErrorCallback);
	if (!glfwInit())  {
		spdlog::error("Failed to initalize GLFW!");

		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,  true);

	auto monitor = glfwGetPrimaryMonitor();
	auto vidMode = glfwGetVideoMode(monitor);

	window = glfwCreateWindow(vidMode->width, vidMode->height, "Syzyf Engine", nullptr, nullptr);
	if (window == NULL) {
		spdlog::error("Failed to create GLFW Window!");

		return false;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	bool err = !gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	
	if (err) {
		spdlog::error("Failed to initialize OpenGL loader!");
		
		return false;
	}

  // Jolt
  JPH::RegisterDefaultAllocator();
  JPH::Factory::sInstance = new JPH::Factory();
  JPH::RegisterTypes();

  JPH::Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
  JPH::AssertFailed = AssertFailedImpl;
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

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImGui::StyleColorsDark();

	return true;
}

void Engine::Terminate() {
	if (rootScene) {
		delete rootScene;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Engine::Update() {
	Time::Update();

	rootScene->Update();
}

void Engine::Render() {
	int display_w, display_h;
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &display_w, &display_h);

	rootScene->GetGraphics()->UpdateScreenResolution(glm::vec2(display_w, display_h));

	rootScene->Render();
}

void Engine::DrawImGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	static ImVec2 window_pos(0, 0);
	static float item_width = 230;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

	rootScene->DrawImGui();

	ImGui::End();

	ImGui::Render();
	int display_w, display_h;
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &display_w, &display_h);

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
	while (!glfwWindowShouldClose(window)) {
		Update();

		Render();

		DrawImGui();

		glfwPollEvents();
		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}
}

void Engine::Exit(int code) {
	Terminate();

	exit(code);
}

Scene* Engine::GetRoot() {
	return rootScene;
}

GLFWwindow* Engine::GetWindow() {
	return window;
}
