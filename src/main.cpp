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
#include <Resources.h>
#include <Light.h>

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
		case GL_DEBUG_SEVERITY_HIGH:         spdlog::error("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
		case GL_DEBUG_SEVERITY_MEDIUM:
		case GL_DEBUG_SEVERITY_LOW:          spdlog::warn("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: spdlog::info("GL {} {}: {} ({})", sourceString, typeString, message, id); break;
	}
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
			movement += this->GlobalTransform().Right();
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			movement += this->GlobalTransform().Left();
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

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			auto lights = GetScene()->FindObjectsOfType<Light>();
			if (lights.size() > 0) {
				delete lights[0];
			}
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

class Stars : public GameObject {
private:
	Mesh* starMesh;
	Material* starMaterial;
	int starCount;
public:
	Stars(int starCount = 1000) {
		this->starMesh = Resources::Get<Mesh>("./res/models/star.obj");
		
		ShaderProgram* starProgram = ShaderProgram::Build()
		.WithVertexShader(
			Resources::Get<VertexShader>("./res/shaders/star.vert")
		).WithPixelShader(
			Resources::Get<PixelShader>("./res/shaders/star.frag")
		).Link();
		starProgram->SetIgnoresDepthPrepass(true);
		starProgram->SetCastsShadows(false);

		this->starMaterial = new Material(starProgram);
		this->starCount = starCount;
	}

	void Render() {
		GetScene()->GetGraphics()->DrawMeshInstanced(
			this->starMesh,
			0,
			this->starMaterial,
			this->GlobalTransform(),
			this->starCount
		);
	}
};

void InitScene() {
	ShaderProgram* meshProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/lambert.frag")
	).Link();

	ShaderProgram* haloProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/halo.frag")
	).Link();
	haloProg->SetCastsShadows(false);

	ShaderProgram* skyProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/skybox.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/skybox.frag")
	).Link();

	ShaderProgram* floorProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/terrain/terrain_lit.vert")
	).WithTessControlShader(
		Resources::Get<TesselationControlShader>("./res/shaders/terrain/terrain_lit.tess_ctrl")
	).WithTessEvaluationShader(
		Resources::Get<TesselationEvaluationShader>("./res/shaders/terrain/terrain_lit.tess_eval")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/terrain/terrain_lit.frag")
	).Link();
	floorProg->SetCastsShadows(false);
	floorProg->SetIgnoresDepthPrepass(true);

	ShaderProgram* coloredProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/lambert color.frag")
	).Link();

	Mesh* floorMesh = Resources::Get<Mesh>("./res/models/floor.obj");
	Mesh* shadeMesh = Resources::Get<Mesh>("./res/models/shade.obj");
	Mesh* cube = Resources::Get<Mesh>("./res/models/not_cube.obj");
	Mesh* roomMesh = Resources::Get<Mesh>("./res/models/room.obj");

	Texture2D* stoneTex = Resources::Get<Texture2D>("./res/textures/lufis.jpeg", TextureFormat::RGB);
	stoneTex->SetMagFilter(GL_LINEAR);
	stoneTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);

	Texture2D* floorTex = Resources::Get<Texture2D>("./res/textures/stone.jpg", TextureFormat::RGB);
	floorTex->SetMagFilter(GL_LINEAR);
	floorTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);
	floorTex->SetWrapModeU(GL_REPEAT);
	floorTex->SetWrapModeV(GL_REPEAT);

	Texture2D* terrainDisplacementTex = Resources::Get<Texture2D>("./res/textures/terrain/Rugged Terrain Height Map PNG.png", TextureFormat::RGB);
	terrainDisplacementTex->SetMagFilter(GL_LINEAR);
	terrainDisplacementTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);
	terrainDisplacementTex->SetWrapModeU(GL_REPEAT);
	terrainDisplacementTex->SetWrapModeV(GL_REPEAT);

	Texture2D* shadedTex = Resources::Get<Texture2D>("./res/testing/uwu.jpg", TextureFormat::RGB);

	if (!shadedTex) {
		shadedTex = Resources::Get<Texture2D>("./res/textures/lufis.jpeg", TextureFormat::RGB);
	}
	shadedTex->SetMagFilter(GL_LINEAR);
	shadedTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);

	Cubemap* skyCubemap = Resources::Get<Cubemap>("./res/textures/skybox.jpg", TextureFormat::RGB);

	Material* terrainMat = new Material(floorProg);
	terrainMat->SetValue<glm::vec3>("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
	terrainMat->SetValue<Texture2D>("heightMap", terrainDisplacementTex);
	terrainMat->SetValue<float>("heightMapScale", 0.4f);

	Material* floorMat = new Material(coloredProg);
	floorMat->SetValue<glm::vec3>("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
	floorMat->SetValue<float>("specularValue", 0.0f);

	Material* shadeMat = new Material(coloredProg);
	shadeMat->SetValue<glm::vec3>("uColor", {0.0f, 0.8f, 0.0f});
	shadeMat->SetValue<float>("specularValue", 0.0f);

	Material* roomMat = new Material(coloredProg);
	roomMat->SetValue<glm::vec3>("uColor", {0.8f, 0.8f, 0.0f});
	roomMat->SetValue<float>("specularValue", 0.0f);

	Material* roomClutterMat = new Material(coloredProg);
	roomClutterMat->SetValue<glm::vec3>("uColor", {0.8f, 0.0f, 0.8f});
	roomClutterMat->SetValue<float>("specularValue", 100.0f);

	Material* centerMat = new Material(meshProg);
	Material* haloMat = new Material(haloProg);
	Material* shadedMat = new Material(meshProg);

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	centerMat->SetValue<glm::vec3>("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
	centerMat->SetValue<Texture2D>("colorTex", stoneTex);

	shadedMat->SetValue<glm::vec3>("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
	shadedMat->SetValue<Texture2D>("colorTex", shadedTex);

	mainScene = new Scene();
	
	auto floorObject = mainScene->CreateNode();
	auto floorRenderer = floorObject->AddObject<MeshRenderer>(floorMesh, floorMat);
	floorObject->LocalTransform().Scale() = glm::vec3(100, 1, 100);
	floorObject->LocalTransform().Position() = {0, -1, 0};

	auto terrainNode = mainScene->CreateNode();
	auto terrainRenderer = terrainNode->AddObject<MeshRenderer>(floorMesh, terrainMat);
	// terrainNode->LocalTransform().Scale() *= 0.5f;
	terrainNode->LocalTransform().Position() = {-5.0f, -0.5f, -5.0f};

	auto shadeNode = mainScene->CreateNode();
	shadeNode->AddObject<MeshRenderer>(shadeMesh, shadeMat);
	shadeNode->LocalTransform().Position() = {-5, 0, 0};
	shadeNode->LocalTransform().Rotation() *= glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));

	auto hiddenNode = mainScene->CreateNode();
	hiddenNode->AddObject<MeshRenderer>(cube, shadedMat);
	hiddenNode->LocalTransform().Position() = {-5, 0, 0};
	
	auto roomNode = mainScene->CreateNode();
	roomNode->AddObject<MeshRenderer>(roomMesh, roomMat)->SetMaterial(roomClutterMat, 1);
	roomNode->LocalTransform().Position() = {5, 0, 0};

	auto notCubeNode = mainScene->CreateNode();
	notCubeNode->GlobalTransform().Position() = glm::zero<glm::vec3>();
	MeshRenderer* centerRenderer = notCubeNode->AddObject<MeshRenderer>(cube, centerMat);
	notCubeNode->AddObject<AutoRotator>(0.1f);

	auto cameraObject = mainScene->CreateNode();
	Camera* camera = cameraObject->AddObject<Camera>(Camera::Perspective(40.0f, 16.0f/9.0f, 1.0f, 100.0f));
	camera->LocalTransform().Position() = glm::vec3(0.0f, 0.0f, -10.0f);
	cameraObject->AddObject<Mover>();

	SceneNode* lightObject = mainScene->CreateNode();
	Light* light = lightObject->AddObject<Light>(Light::PointLight(glm::vec3(1, 1, 1), 10, 0.5));
	light->GlobalTransform().Position() = glm::vec3(0, 0, -2);

	SceneNode* spotLightNode = mainScene->CreateNode();
	Light* spotLight = spotLightNode->AddObject<Light>(Light::SpotLight(glm::vec3(0, 0, 1), glm::radians(45.0f), 10, 1));
	spotLight->GlobalTransform().Position() = {-5.0f, 0.8f, -3.0f};
	spotLight->GlobalTransform().Rotation() *= glm::angleAxis(glm::radians(15.0f), glm::vec3(1, 0, 0));
	spotLight->SetShadowCasting(true);

	SceneNode* pointLightNode = mainScene->CreateNode();
	Light* pointLight = pointLightNode->AddObject<Light>(Light::PointLight(glm::vec3(1, 1, 1), 5, 1));
	pointLight->GlobalTransform().Position() = {5.0f, -0.2f, -1.0f};
	pointLight->SetShadowCasting(true);
	
	auto skyboxObject = mainScene->CreateNode(notCubeNode);
	skyboxObject->AddObject<Stars>(1000);
	// skyboxObject->AddObject<Skybox>(skyMat);
}

int main(int, char**) {
	if (!InitProgram()) {
		spdlog::error("Failed to initialize project!");
		return EXIT_FAILURE;
	}
	spdlog::info("Initialized project.");

	InitScene();

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

	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,  true);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "GLGP Project", NULL, NULL);
	if (window == NULL) {
		spdlog::error("Failed to create GLFW Window!");
		return false;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	bool err = !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
	int contextFlags = 0;
	glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);

	if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	
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

	mainScene->GetGraphics()->UpdateScreenResolution(glm::vec2(display_w, display_h));

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
