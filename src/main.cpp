#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#include <stdio.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <Shader.h>
#include <Mesh.h>
#include <Material.h>
#include <MeshRenderer.h>
#include <Scene.h>
#include <Camera.h>

static void GLFWErrorCallback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool InitProgram();
void InitImgui();

void Input();
void Update();
void Render();

void ImGuiBegin();
void ImGuiUpdate();
void ImGuiRender();

void EndFrame();

constexpr int32_t WINDOW_WIDTH  = 1920;
constexpr int32_t WINDOW_HEIGHT = 1080;

GLFWwindow* window = nullptr;

const     char*   glsl_version     = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

Scene* mainScene;

int main(int, char**) {
	if (!InitProgram()) {
		spdlog::error("Failed to initialize project!");
		return EXIT_FAILURE;
	}
	spdlog::info("Initialized project.");

	VertexShader* vert = (VertexShader*) ShaderBase::Load("./res/shaders/basic.vert");
	PixelShader* frag = (PixelShader*) ShaderBase::Load("./res/shaders/basic.frag");

	ShaderProgram* prog = ShaderProgram::Build().WithVertexShader(vert).WithPixelShader(frag).Link();

	Mesh cube = Mesh::Load("./res/models/cube.obj", VertexSpec::Mesh);

	Material* mat = new Material(prog);

	glm::mat4 model = glm::identity<glm::mat4>();
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::zero<glm::vec3>(), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 projection = glm::perspective(glm::radians(40.0f), 16.0f/9.0f, 1.0f, 100.0f);

	mat->SetValue<glm::mat4>("gMVPMatrix", projection * view * model);
	mat->SetValue<glm::vec3>("uColor", glm::vec3(1.0f, 1.0f, 1.0f));

	mainScene = new Scene();
	
	auto rendererObject = mainScene->CreateNode();
	auto cameraObject = mainScene->CreateNode();

	MeshRenderer* renderer = rendererObject->AddObject<MeshRenderer>(cube, mat);

	Camera* camera = cameraObject->AddObject<Camera>(glm::radians(40.0f), 16.0f/9.0f, 1.0f, 100.0f);

	camera->GetTransform().LocalTransform().Position() = glm::vec3(0.0f, 0.0f, -10.0f);

	InitImgui();
	spdlog::info("Initialized ImGui.");

	while (!glfwWindowShouldClose(window)) {
		Input();

		Update();

		Render();

		ImGuiBegin();
		ImGuiUpdate();
		ImGuiRender();

		EndFrame();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

bool InitProgram() {
	glfwSetErrorCallback(GLFWErrorCallback);
	if (!glfwInit())  {
		spdlog::error("Failed to initalize GLFW!");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "GLGP Project", NULL, NULL);
	if (window == NULL) {
		spdlog::error("Failed to create GLFW Window!");
		return false;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	
	bool err = !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
	glEnable(GL_DEPTH_TEST);
	
	if (err) {
		spdlog::error("Failed to initialize OpenGL loader!");
		return false;
	}

	return true;
}

void InitImgui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImGui::StyleColorsDark();
}

void Input() {
	
}

void Update() {
	mainScene->Update();
}

void Render() {
	int display_w, display_h;
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &display_w, &display_h);

	glViewport(0, 0, display_w, display_h);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mainScene->Render();
}

void ImGuiBegin() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiUpdate() {

}

void ImGuiRender() {
	ImGui::Render();
	int display_w, display_h;
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &display_w, &display_h);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EndFrame() {
	glfwPollEvents();
	glfwMakeContextCurrent(window);
	glfwSwapBuffers(window);
}

