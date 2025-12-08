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

void InitScene() {
	// const unsigned int textureSize = 512;

	// Texture2D* computeTexture = new Texture2D(textureSize, textureSize, TextureFormat::RGBAFloat);
	// computeTexture->SetMagFilter(GL_NEAREST);

	// ComputeShader* comp = (ComputeShader*) ShaderBase::Load("./res/shaders/forwardplus/fullscreenquad.comp");
	// ComputeShaderDispatch* quadDispatch = new ComputeShaderDispatch(comp);

	// quadDispatch->GetData()->SetValue("imgOutput", computeTexture);
	// quadDispatch->GetData()->SetUniformBuffer("FullScreenQuadParams", glm::vec2(0.2f, 0.7f));

	// quadDispatch->Dispatch(textureSize, textureSize, 1);

	// ShaderProgram* quadProg = ShaderProgram::Build()
	// .WithVertexShader(
	// 	(VertexShader*) ShaderBase::Load("./res/shaders/basic.vert")
	// )
	// .WithPixelShader(
	// 	(PixelShader*) ShaderBase::Load("./res/shaders/basic.frag")
	// ).Link();

	// float offset = 1.0f;
	// GLuint bufHandle = 0;

	// glGenBuffers(1, &bufHandle);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufHandle);
	// glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float), &offset, GL_STATIC_DRAW);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Mesh quadMesh = Mesh::Load("./res/models/fullscreenquad.obj", VertexSpec::Mesh);
	// Material* quadMat = new Material(quadProg);
	// quadMat->SetValue("uColor", glm::one<glm::vec3>());
	// quadMat->SetValue("colorTex", computeTexture);
	// quadMat->BindStorageBuffer("in_Offsets", bufHandle);

	// mainScene = new Scene();

	// SceneNode* quadObject = mainScene->CreateNode();
	// quadObject->AddObject<MeshRenderer>(quadMesh, quadMat);
	// quadObject->AddObject<Camera>(Camera::Orthographic(glm::vec2(3.0f, 3.0f)));

	VertexShader* meshVert = Resources::Get<VertexShader>("./res/shaders/lit.vert");
	PixelShader* meshFrag = Resources::Get<PixelShader>("./res/shaders/lambert.frag");

	ShaderProgram* meshProg = ShaderProgram::Build().WithVertexShader(meshVert).WithPixelShader(meshFrag).Link();

	VertexShader* skyVert = Resources::Get<VertexShader>("./res/shaders/skybox.vert");
	PixelShader* skyFrag = Resources::Get<PixelShader>("./res/shaders/skybox.frag");

	ShaderProgram* skyProg = ShaderProgram::Build().WithVertexShader(skyVert).WithPixelShader(skyFrag).Link();

	ShaderProgram* floorProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/lambert color.frag")
	).Link();

	Mesh* floorMesh = Resources::Get<Mesh>("./res/models/floor.obj", VertexSpec::Mesh);

	Material* floorMat = new Material(floorProg);
	floorMat->SetValue<glm::vec3>("uColor", {1, 1, 1});

	Mesh* cube = Resources::Get<Mesh>("./res/models/cube.obj", VertexSpec::Mesh);
	Material* centerMat = new Material(meshProg);
	Material* orbiterMat = new Material(meshProg);

	Texture2D* stoneTex = Resources::Get<Texture2D>("./res/textures/lufis.jpeg", TextureFormat::RGB);
	stoneTex->SetMagFilter(GL_LINEAR);
	stoneTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);

	Texture2D* invStoneTex = Resources::Get<Texture2D>("./res/textures/stone_inv.jpg", TextureFormat::RGB);
	invStoneTex->SetMagFilter(GL_LINEAR);
	invStoneTex->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);

	Cubemap* skyCubemap = Resources::Get<Cubemap>("./res/textures/skybox.jpg", TextureFormat::RGB);

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	centerMat->SetValue<glm::vec3>("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
	centerMat->SetValue<Texture2D>("colorTex", stoneTex);
	orbiterMat->SetValue<glm::vec3>("uColor", glm::vec3(0.0f, 0.5f, 1.0f));
	orbiterMat->SetValue<Texture2D>("colorTex", invStoneTex);

	mainScene = new Scene();
	
	auto floorObject = mainScene->CreateNode();
	auto floorRenderer = floorObject->AddObject<MeshRenderer>(floorMesh, floorMat);
	floorObject->LocalTransform().Scale() *= 30;
	floorObject->LocalTransform().Position() = {0, -1, 0};

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

	SceneNode* lightObject = mainScene->CreateNode(cameraObject);
	// Light* light = lightObject->AddObject<Light>(Light::PointLight(glm::vec3(1.0, 1.0, 1.0), 100.0, 2.0));

	for (int i = 0; i < 1; i++) {
		auto lanternObj = mainScene->CreateNode();
		Light* lantern = lanternObj->AddObject<Light>(Light::PointLight(glm::vec3(1.0, 1.0, 1.0), 100.0, 2.0));

		lantern->LocalTransform().Position() = glm::vec3{
			-6 + (i % 2) * 12,
			0,
			-6 + (i / 2) * 12
		};

		auto vis = mainScene->CreateNode(lanternObj);
		vis->LocalTransform().Scale() *= 0.1;
		MeshRenderer* visRenderer = vis->AddObject<MeshRenderer>(cube, centerMat);
	}

	auto skyboxObject = mainScene->CreateNode();
	skyboxObject->AddObject<Skybox>(skyMat);
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
