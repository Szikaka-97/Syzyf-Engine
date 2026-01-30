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
#include <Graphics.h>
#include <Camera.h>
#include <Skybox.h>
#include <Resources.h>
#include <Light.h>
#include <Bloom.h>
#include <ReflectionProbe.h>
#include <ReflectionProbeSystem.h>
#include <Tonemapper.h>
#include <Debug.h>

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
private:
	float pitch;
	float rotation;
public:
	Mover() {
		this->pitch = 0;
		this->rotation = 0;
	}

	void Update() {
		glm::vec3 movement = glm::zero<glm::vec3>();
		glm::quat rotation = glm::identity<glm::quat>();

		glm::vec3 right = this->GlobalTransform().Right();
		glm::vec3 up = glm::vec3(0, 1, 0);
		glm::vec3 forward = glm::cross(right, up);

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			movement += right;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			movement -= right;
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			movement += forward;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			movement -= forward;
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			this->rotation -= 1.0f;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			this->rotation += 1.0f;
		}
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			this->pitch -= 1.0f;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			this->pitch += 1.0f;
		}

		if (this->rotation < -180) {
			this->rotation += 360;
		}
		else if (this->rotation > 180) {
			this->rotation -= 360;
		}

		this->pitch = glm::clamp(this->pitch, -89.0f, 89.0f);

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			auto lights = GetScene()->FindObjectsOfType<Light>();
			if (lights.size() > 0) {
				delete lights[0];
			}
		}

		this->GlobalTransform().Position() += movement * 0.04f;
		this->GlobalTransform().Rotation() = glm::angleAxis(
			glm::radians(this->rotation), glm::vec3(0, 1, 0)
		) * glm::angleAxis(glm::radians(this->pitch), glm::vec3(1, 0, 0));
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
		).WithGeometryShader(
			Resources::Get<GeometryShader>("./res/shaders/star.geom")
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
	mainScene = new Scene();
	
	ShaderProgram* skyProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/skybox.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/skybox.frag")
	).Link();

	ShaderProgram* coloredProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/lambert color.frag")
	).Link();

	ShaderProgram* pbrProg = ShaderProgram::Build().WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/pbr.frag")
	).Link();

	Mesh* gmConstructMesh = Resources::Get<Mesh>("./res/models/construct/construct.obj", true);
	Mesh* cannonMesh = Resources::Get<Mesh>("./res/models/cannon/cannon.obj");
	Mesh* cubeMesh = Resources::Get<Mesh>("./res/models/not_cube.obj");

	Cubemap* skyCubemap = Resources::Get<Cubemap>("./res/textures/moonless_golf_4k.hdr", Texture::HDRColorBuffer);
	skyCubemap->SetWrapModeU(TextureWrap::Clamp);
	skyCubemap->SetWrapModeV(TextureWrap::Clamp);
	skyCubemap->SetWrapModeW(TextureWrap::Clamp);

	Texture2D* cannonDiffuse = Resources::Get<Texture2D>("./res/models/cannon/textures/cannon_01_diff_1k.png", Texture::ColorTextureRGB);
	Texture2D* cannonNormal = Resources::Get<Texture2D>("./res/models/cannon/textures/cannon_01_nor_gl_1k.png", Texture::TechnicalMapXYZ);
	Texture2D* cannonARM = Resources::Get<Texture2D>("./res/models/cannon/textures/cannon_01_arm_1k.png", Texture::TechnicalMapXYZ);
	
	Texture2D* reflectiveDiffuse = Resources::Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-albedo.png", Texture::ColorTextureRGB);
	Texture2D* reflectiveNormal = Resources::Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-Normal-ogl.png", Texture::TechnicalMapXYZ);
	Texture2D* reflectiveARM = Resources::Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-arm.png", Texture::TechnicalMapXYZ);
	Texture2D* roughARM = Resources::Get<Texture2D>("./res/textures/material_preview/worn-rough-metal-arm.png", Texture::TechnicalMapXYZ);
	Texture2D* shinyNonMetalARM = Resources::Get<Texture2D>("./res/textures/material_preview/worn-shiny-nonmetal-arm.png", Texture::TechnicalMapXYZ);

	Material* cannonMat = new Material(pbrProg);
	cannonMat->SetValue("albedoMap", cannonDiffuse);
	cannonMat->SetValue("normalMap", cannonNormal);
	cannonMat->SetValue("armMap", cannonARM);

	Material* reflectiveMat = new Material(pbrProg);
	reflectiveMat->SetValue("albedoMap", reflectiveDiffuse);
	reflectiveMat->SetValue("normalMap", reflectiveNormal);
	reflectiveMat->SetValue("armMap", reflectiveARM);

	Material* roughMat = new Material(pbrProg);
	roughMat->SetValue("albedoMap", reflectiveDiffuse);
	roughMat->SetValue("normalMap", reflectiveNormal);
	roughMat->SetValue("armMap", roughARM);

	Material* shinyMat = new Material(pbrProg);
	shinyMat->SetValue("albedoMap", reflectiveDiffuse);
	shinyMat->SetValue("normalMap", reflectiveNormal);
	shinyMat->SetValue("armMap", shinyNonMetalARM);

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	auto constructNode = mainScene->CreateNode("gm_construct");
	constructNode->AddObject<MeshRenderer>(gmConstructMesh, gmConstructMesh->GetDefaultMaterials());

	auto cannonNode = mainScene->CreateNode("Cannon");
	cannonNode->AddObject<MeshRenderer>(cannonMesh, cannonMat);

	auto cubeNode = mainScene->CreateNode("Reflective Cube");
	cubeNode->AddObject<MeshRenderer>(cubeMesh, reflectiveMat);
	cubeNode->GlobalTransform().Position() = {-2.0f, 1.0f, 0.0f};
	cubeNode->GlobalTransform().Scale() = glm::vec3(0.6f);

	auto roughCubeNode = mainScene->CreateNode(cubeNode, "Rough Cube");
	roughCubeNode->AddObject<MeshRenderer>(cubeMesh, roughMat);
	roughCubeNode->LocalTransform().Position() = {0, 0, 3};

	auto shinyCubeNode = mainScene->CreateNode(cubeNode, "Shiny Cube");
	shinyCubeNode->AddObject<MeshRenderer>(cubeMesh, shinyMat);
	shinyCubeNode->LocalTransform().Position() = {0, 0, -3};

	auto cubeNode2 = mainScene->CreateNode("Reflective Cube");
	cubeNode2->AddObject<MeshRenderer>(cubeMesh, reflectiveMat);
	cubeNode2->GlobalTransform().Position() = {-25.0f, 1.0f, 0.0f};
	cubeNode2->GlobalTransform().Scale() = glm::vec3(0.6f);

	auto roughCubeNode2 = mainScene->CreateNode(cubeNode2, "Rough Cube");
	roughCubeNode2->AddObject<MeshRenderer>(cubeMesh, roughMat);
	roughCubeNode2->LocalTransform().Position() = {0, 0, 3};

	auto shinyCubeNode2 = mainScene->CreateNode(cubeNode2, "Shiny Cube");
	shinyCubeNode2->AddObject<MeshRenderer>(cubeMesh, shinyMat);
	shinyCubeNode2->LocalTransform().Position() = {0, 0, -3};

	auto cameraNode = mainScene->CreateNode("Camera");
	Camera* camera = cameraNode->AddObject<Camera>(Camera::Perspective(40.0f, 16.0f/9.0f, 0.5f, 100.0f));
	camera->LocalTransform().Position() = glm::vec3(0.0f, 1.5f, -10.0f);
	cameraNode->AddObject<Mover>();

	auto skyboxNode = mainScene->CreateNode(constructNode, "Floor");
	skyboxNode->AddObject<Skybox>(skyMat);

	auto lightNode = mainScene->CreateNode("Point Light");
	lightNode->AddObject<Light>(Light::PointLight({1, 1, 1}, 10, 2))->SetShadowCasting(true);
	lightNode->GlobalTransform().Position() = {-1, 2.2f, 0};

	auto lightNode2 = mainScene->CreateNode("Directional Light");
	lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 2))->SetShadowCasting(true);
	lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
	lightNode2->GlobalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(50.0f, -20.0f, 0.0f)));

	auto envProbe = mainScene->CreateNode(cubeNode, "Reflection Probe");
	envProbe->AddObject<ReflectionProbe>();

	auto envProbe2 = mainScene->CreateNode("Reflection Probe");
	envProbe2->AddObject<ReflectionProbe>();
	envProbe2->GlobalTransform().Position() = {-10.0f, 1.5f, 0.6f};

	auto envProbe3 = mainScene->CreateNode("Reflection Probe");
	envProbe3->AddObject<ReflectionProbe>();
	envProbe3->GlobalTransform().Position() = {-29.0f, 1.5f, 0.6f};

	cameraNode->AddObject<Bloom>();
	cameraNode->AddObject<Tonemapper>()->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);

	mainScene->AddComponent<DebugInspector>();
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
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

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
	static ImVec2 window_pos(0, 0);
	static float item_width = 230;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

	mainScene->DrawImGui();

	ImGui::End();
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
