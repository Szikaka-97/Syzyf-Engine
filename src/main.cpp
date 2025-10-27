#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#include <stdio.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <Formatters.h>
#include <Shader.h>
#include <Mesh.h>
#include <Material.h>
#include <MeshRenderer.h>
#include <Scene.h>
#include <Camera.h>
#include <Skybox.h>

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

class Mover : public GameObject {
public:
	void Update() {
		glm::vec3 movement = glm::zero<glm::vec3>();
		glm::quat rotation = glm::identity<glm::quat>();

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			movement += this->GlobalTransform().Left();
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			movement += this->GlobalTransform().Right();
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			movement += this->GlobalTransform().Forward();
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			movement += this->GlobalTransform().Backward();
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			rotation *= glm::angleAxis(glm::radians(-1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			rotation *= glm::angleAxis(glm::radians(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		this->GlobalTransform().Position() += movement * 0.04f;
		this->GlobalTransform().Rotation() *= rotation;
	}
};

class AutoRotator : public GameObject {
private:
	float speed;
public:
	AutoRotator(float speed) {
		this->speed = speed;
	}

	void Update() {
		glm::quat rotation = glm::angleAxis(glm::radians(this->speed), glm::vec3(0.0f, 1.0f, 0.0f));

		this->LocalTransform().Rotation() *= rotation;
	}
};

int main(int, char**) {
	if (!InitProgram()) {
		spdlog::error("Failed to initialize project!");
		return EXIT_FAILURE;
	}
	spdlog::info("Initialized project.");

	VertexShader* meshVert = (VertexShader*) ShaderBase::Load("./res/shaders/basic.vert");
	PixelShader* meshFrag = (PixelShader*) ShaderBase::Load("./res/shaders/basic.frag");

	ShaderProgram* meshProg = ShaderProgram::Build().WithVertexShader(meshVert).WithPixelShader(meshFrag).Link();

	VertexShader* skyVert = (VertexShader*) ShaderBase::Load("./res/shaders/skybox.vert");
	PixelShader* skyFrag = (PixelShader*) ShaderBase::Load("./res/shaders/skybox.frag");

	ShaderProgram* skyProg = ShaderProgram::Build().WithVertexShader(skyVert).WithPixelShader(skyFrag).Link();

	Mesh cube = Mesh::Load("./res/models/cube.obj", VertexSpec::Mesh);

	Material* centerMat = new Material(meshProg);
	Material* orbiterMat = new Material(meshProg);

	Texture2D* stoneTex = Texture::Load<Texture2D>("./res/textures/stone.jpg", TextureFormat::RGB);
	stoneTex->SetMagFilter(GL_LINEAR);
	stoneTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);

	Texture2D* invStoneTex = Texture::Load<Texture2D>("./res/textures/stone_inv.jpg", TextureFormat::RGB);
	invStoneTex->SetMagFilter(GL_LINEAR);
	invStoneTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);

	Cubemap* skyCubemap = Texture::Load<Cubemap>("./res/textures/skybox.jpg", TextureFormat::RGB);

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	centerMat->SetValue<glm::vec3>("uColor", glm::vec3(0.0f, 0.5f, 1.0f));
	centerMat->SetValue<Texture2D>("colorTex", stoneTex);
	orbiterMat->SetValue<glm::vec3>("uColor", glm::vec3(0.0f, 0.5f, 1.0f));
	orbiterMat->SetValue<Texture2D>("colorTex", invStoneTex);

	mainScene = new Scene();
	
	auto rendererObject = mainScene->CreateNode();
	auto cameraObject = mainScene->CreateNode();

	rendererObject->AddObject<MeshRenderer>(cube, centerMat);
	rendererObject->AddObject<AutoRotator>(1.0f);

	auto rendererChildRotator = mainScene->CreateNode(rendererObject);
	rendererChildRotator->AddObject<AutoRotator>(-2.0f);

	for (int i = 0; i < 4; i++) {
		auto rendererChild = mainScene->CreateNode(rendererChildRotator);

		rendererChild->LocalTransform().Scale() = glm::vec3(0.3f);
		rendererChild->LocalTransform().Position() = glm::vec3(0.0f, 0.0f, 1.2f) * glm::angleAxis(glm::radians(90.0f * i), glm::vec3(0.0f, 1.0f, 0.0f));

		rendererChild->AddObject<AutoRotator>(1.0f);
		
		rendererChild->AddObject<MeshRenderer>(cube, orbiterMat);
	}

	Camera* camera = cameraObject->AddObject<Camera>(Camera::Perspective(40.0f, 16.0f/9.0f, 1.0f, 100.0f));
	cameraObject->AddObject<Mover>();

	camera->LocalTransform().Position() = glm::vec3(0.0f, 0.0f, -10.0f);

	auto skyboxObject = mainScene->CreateNode();
	skyboxObject->AddObject<Skybox>(skyMat);

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

