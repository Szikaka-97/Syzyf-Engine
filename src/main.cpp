#include "GltfImporter.h"

#include "TimeSystem.h"
#include "animation/AnimationSystem.h"
#include "imgui.h"
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
#include <InputSystem.h>
#include <Engine.h>
#include <glm/trigonometric.hpp>
#include <spdlog/spdlog.h>

class AnimatedThingTag : public GameObject {};
#include <Viewport.h>

class Mover : public GameObject, public ImGuiDrawable {
private:
	float pitch;
	float rotation;
	bool movementEnabled;
	int mode;
	float movementSpeed = 10.0f;
	float mouseSensitivity = 1.0f;
public:
	Mover() {
		this->pitch = 0;
		this->rotation = 0;
		this->mode = 0;
	}

	void Update() {
		if (movementEnabled) {
			glm::vec3 movement = glm::zero<glm::vec3>();
			glm::quat rotation = glm::identity<glm::quat>();

			glm::vec3 right = this->GlobalTransform().Right();
			glm::vec3 up = glm::vec3(0, 1, 0);
			glm::vec3 forward = mode == 0 ? glm::cross(right, up) : this->GlobalTransform().Forward();

			if (GetScene()->Input()->KeyPressed(Key::A)) {
				movement += right;
			}
			if (GetScene()->Input()->KeyPressed(Key::D)) {
				movement -= right;
			}
			if (GetScene()->Input()->KeyPressed(Key::W)) {
				movement += forward;
			}
			if (GetScene()->Input()->KeyPressed(Key::S)) {
				movement -= forward;
			}

      if (GetScene()->Input()->KeyPressed(Key::Enter)) {
        auto* thing = this->GetScene()->FindObjectsOfType<AnimatedThingTag>().front();
        if (thing) {
          auto* animationObject = thing->GetObject<AnimationComponent>();
          animationObject->Play("pivotAction");
        }
      }
	
			glm::vec2 deltaMovement = GetScene()->Input()->GetMouseMovement();

			this->rotation -= (deltaMovement.x / 20) * this->mouseSensitivity;
			this->pitch -= (deltaMovement.y / 20) * this->mouseSensitivity;

			if (this->rotation < -180) {
				this->rotation += 360;
			}
			else if (this->rotation > 180) {
				this->rotation -= 360;
			}

			this->pitch = glm::clamp(this->pitch, -89.0f, 89.0f);
			this->GlobalTransform().Position() += movement * this->movementSpeed * Time::Delta();
			this->GlobalTransform().Rotation() = glm::angleAxis(
				glm::radians(this->rotation), glm::vec3(0, 1, 0)
			) * glm::angleAxis(glm::radians(this->pitch), glm::vec3(1, 0, 0));
		}

		if (GetScene()->Input()->KeyDown(Key::Escape)) {
			this->movementEnabled = !this->movementEnabled;

			GetScene()->Input()->SetMouseLocked(this->movementEnabled);
		}
	}

	virtual void DrawImGui() {
		const char* modes[] { "Walking", "Freecam", };

		ImGui::Combo("Movement type", &this->mode, modes, 2);

		ImGui::InputFloat("Movement speed", &this->movementSpeed);
		ImGui::InputFloat("Mouse sensitivity", &this->mouseSensitivity);
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

class Stars : public GameObject, public ImGuiDrawable {
private:
	Mesh* starMesh;
	Material* starMaterial;
	int starCount;
public:
	Stars(int starCount = 1000) {
		this->starMesh = GetScene()->Resources()->Get<Mesh>("./res/models/star.obj");
		
		ShaderProgram* starProgram = ShaderProgram::Build()
		.WithVertexShader(
			GetScene()->Resources()->Get<VertexShader>("./res/shaders/star.vert")
		).WithGeometryShader(
			GetScene()->Resources()->Get<GeometryShader>("./res/shaders/star.geom")
		).WithPixelShader(
			GetScene()->Resources()->Get<PixelShader>("./res/shaders/star.frag")
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
			this->starCount,
			BoundingBox::CenterAndExtents(glm::vec3(0, 0, 0), glm::vec3(15, 15, 15))
		);
	}

	void DrawImGui() {
		ImGui::InputInt("Star count", &this->starCount);
	}
};

void InitScene(Scene* mainScene) {
	ShaderProgram* skyProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/skybox.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/skybox.frag")
	).Link();

	ShaderProgram* coloredProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/lambert color.frag")
	).Link();

	ShaderProgram* diffuseTexProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/lambert.frag")
	).Link();

	ShaderProgram* pbrProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/pbr.frag")
	).Link();

	ShaderProgram* transparentProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/transparent.frag")
	).Link();
	transparentProg->SetTransparent(true);

	// Mesh* gmConstructMesh = mainScene->Resources()->Get<Mesh>("./res/models/construct/construct.obj", true);
	Mesh* cannonMesh = mainScene->Resources()->Get<Mesh>("./res/models/cannon/cannon.obj");
	Mesh* cubeMesh = mainScene->Resources()->Get<Mesh>("./res/models/not_cube.obj");
	Mesh* tvMesh = mainScene->Resources()->Get<Mesh>("./res/models/tv_stand.fbx");
	Mesh* schnozMesh = mainScene->Resources()->Get<Mesh>("./res/models/schnoz/schnoz.obj");

	Cubemap* skyCubemap = mainScene->Resources()->Get<Cubemap>("./res/textures/citrus_orchard_road_puresky.hdr", Texture::HDRColorBuffer);
	skyCubemap->SetWrapModeU(TextureWrap::Clamp);
	skyCubemap->SetWrapModeV(TextureWrap::Clamp);
	skyCubemap->SetWrapModeW(TextureWrap::Clamp);

	Texture2D* cannonDiffuse = mainScene->Resources()->Get<Texture2D>("./res/models/cannon/textures/cannon_01_diff_1k.png", Texture::ColorTextureRGB);
	Texture2D* cannonNormal = mainScene->Resources()->Get<Texture2D>("./res/models/cannon/textures/cannon_01_nor_gl_1k.png", Texture::TechnicalMapXYZ);
	Texture2D* cannonARM = mainScene->Resources()->Get<Texture2D>("./res/models/cannon/textures/cannon_01_arm_1k.png", Texture::TechnicalMapXYZ);
	
	Texture2D* reflectiveDiffuse = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-albedo.png", Texture::ColorTextureRGB);
	Texture2D* reflectiveNormal = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-Normal-ogl.png", Texture::TechnicalMapXYZ);
	Texture2D* reflectiveARM = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-arm.png", Texture::TechnicalMapXYZ);
	Texture2D* roughARM = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-rough-metal-arm.png", Texture::TechnicalMapXYZ);
	Texture2D* shinyNonMetalARM = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-shiny-nonmetal-arm.png", Texture::TechnicalMapXYZ);

	Texture2D* schnozTexture = mainScene->Resources()->Get<Texture2D>("./res/models/schnoz/Diffuse.png", Texture::ColorTextureRGB);

	Viewport* schnozPreview = new Viewport();
	schnozPreview->GetFramebuffer()->CreateColorAttachment(true, false);
	schnozPreview->GetFramebuffer()->CreateDepthAttachment(false, false);
	schnozPreview->SetSize(glm::uvec2(1024, 512));

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

	Material* pinkTransparentMat = new Material(transparentProg);
	pinkTransparentMat->SetValue("uColor", glm::vec4(1.0, 0.5, 0.5, 0.6));

	Material* blueTransparentMat = new Material(transparentProg);
	blueTransparentMat->SetValue("uColor", glm::vec4(0.5, 0.5, 1.0, 0.6));

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	Material* tvMatStand = new Material(coloredProg);
	tvMatStand->SetValue("uColor", glm::vec3(0.8, 0.8, 0.8));

	Material* screenMat = new Material(diffuseTexProg);
	screenMat->SetValue("uColor", glm::vec3(1, 1, 1));
	screenMat->SetValue("colorTex", (Texture2D*) schnozPreview->GetFramebuffer()->GetColorTexture());

	Material* schnozMat = new Material(diffuseTexProg);
	schnozMat->SetValue("uColor", glm::vec3(1, 1, 1));
	schnozMat->SetValue("colorTex", schnozTexture);

	// auto constructNode = mainScene->CreateNode("gm_construct");
	// constructNode->AddObject<MeshRenderer>(gmConstructMesh, gmConstructMesh->GetDefaultMaterials());

	auto cannonNode = mainScene->CreateNode("Cannon");
	cannonNode->AddObject<MeshRenderer>(cannonMesh, cannonMat);

	auto cubeNode = mainScene->CreateNode("Reflective Cube");
	cubeNode->AddObject<MeshRenderer>(cubeMesh, reflectiveMat);
	cubeNode->GlobalTransform().Position() = {-2.0f, 1.0f, 0.0f};
	cubeNode->GlobalTransform().Scale() = glm::vec3(0.6f);

	auto roughCubeNode = mainScene->CreateNode(cubeNode, "Rough Cube");
	roughCubeNode->AddObject<MeshRenderer>(cubeMesh, roughMat);
	roughCubeNode->LocalTransform().Position() = {0, 0, 3};

	auto pinkTransparentCubeNode = mainScene->CreateNode(cubeNode, "Pink Cube");
	pinkTransparentCubeNode->AddObject<MeshRenderer>(cubeMesh, pinkTransparentMat);
	pinkTransparentCubeNode->LocalTransform().Position() = {-3, 0, -3};

	auto blueTransparentCubeNode = mainScene->CreateNode(cubeNode, "Blue Cube");
	blueTransparentCubeNode->AddObject<MeshRenderer>(cubeMesh, blueTransparentMat);
	blueTransparentCubeNode->LocalTransform().Position() = {-3, 0, -5};

	auto cameraNode = mainScene->CreateNode("Camera");
	Camera* camera = cameraNode->AddObject<Camera>(Camera::Perspective(40.0f, 16.0f/9.0f, 0.5f, 200.0f));
	camera->LocalTransform().Position() = glm::vec3(0.0f, 1.5f, -10.0f);
	cameraNode->AddObject<Mover>();
  camera->GetObject<Camera>()->SetAsMainCamera();

	auto skyboxNode = mainScene->CreateNode("Floor");
	skyboxNode->AddObject<Skybox>(skyMat);

	auto lightNode = mainScene->CreateNode("Point Light");
	lightNode->AddObject<Light>(Light::PointLight({1, 1, 1}, 10, 2))->SetShadowCasting(false);
	lightNode->GlobalTransform().Position() = {-1, 2.2f, 0};

	auto lightNode2 = mainScene->CreateNode("Directional Light");
	lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 2))->SetShadowCasting(false);
	lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
	lightNode2->GlobalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(91.0f, 0.0f, 0.0f)));

	auto envProbe = mainScene->CreateNode(cubeNode, "Reflection Probe");
	envProbe->AddObject<ReflectionProbe>();

	auto envProbe2 = mainScene->CreateNode("Reflection Probe");
	envProbe2->AddObject<ReflectionProbe>();
	envProbe2->GlobalTransform().Position() = {-10.0f, 1.5f, 0.6f};

	auto envProbe3 = mainScene->CreateNode("Reflection Probe");
	envProbe3->AddObject<ReflectionProbe>();
	envProbe3->GlobalTransform().Position() = {-29.0f, 1.5f, 0.6f};

	auto starsAttachmentNode = mainScene->CreateNode("Stars Scene Attachment");
	
	auto starsScene = new Scene();

	auto starsNode = starsScene->CreateNode("Stars");
	starsNode->AddObject<Stars>(1000);
	starsNode->GlobalTransform().Position() = {-15.0f, 5.5f, -105.0f};

	starsAttachmentNode->AttachScene(starsScene);

	SceneNode* tvNode = mainScene->CreateNode("TV");
	tvNode->LocalTransform().Scale() = glm::vec3(1.5, 1.5, 1.5);
	tvNode->LocalTransform().Position() = glm::vec3(3, 0, -2);
	tvNode->LocalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(-90.0f, 20.0f, 0.0f)));

	auto tvRenderer = tvNode->AddObject<MeshRenderer>(tvMesh, nullptr);
	tvRenderer->SetMaterial(tvMatStand, 0);
	tvRenderer->SetMaterial(screenMat, 1);
	tvRenderer->SetMaterial(tvMatStand, 2);
	tvRenderer->SetMaterial(tvMatStand, 3);

	SceneNode* schnozCameraNode = mainScene->CreateNode("Schnoz Camera");
	schnozCameraNode->LocalTransform().Position() = glm::vec3(-56.5, 2.0, -2.0);
	schnozCameraNode->LocalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(5.0f, 85.0f, 0.0f)));
	
	auto schnozCamera = schnozCameraNode->AddObject<Camera>(Camera::Perspective(40.0f, 16.0f/9.0f, 0.5f, 200.0f));
	schnozCamera->SetAspectRatio(2);
	schnozCamera->SetRenderTarget(schnozPreview);
	schnozCamera->SetLayerMask(uint8_t(5));

	SceneNode* schnozNode = mainScene->CreateNode("Schnoz");
	schnozNode->LocalTransform().Position() = glm::vec3(-53.5, 1.75, -2.4);
	schnozNode->LocalTransform().Scale() = glm::vec3(0.15, 0.15, 0.15);
	schnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);
	schnozNode->AddObject<AutoRotator>(1);
	schnozNode->SetLayer(5);

	SceneNode* schnozLightNode = mainScene->CreateNode("Schnoz Light");
	schnozLightNode->LocalTransform().Position() = glm::vec3(-55.5, 3.0, -2.0);
	schnozLightNode->AddObject<Light>(Light::PointLight(glm::vec3(1, 1, 1), 5, 5));

	cameraNode->AddObject<Bloom>();
	cameraNode->AddObject<Tonemapper>()->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);

	ShaderProgram* pbrGltfProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/lit_gltf.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/pbr_gltf.frag")
	).Link();
  SceneNode* gltfAttachmentNode = mainScene->CreateNode("Gltf Scene Attachment");
  Scene* gltfScene = GltfImporter::LoadScene("./res/models/animated_cube.glb", "Animated Thing");
  gltfScene->GetRootNode()->AddObject<AnimatedThingTag>();
  starsAttachmentNode->AttachScene(gltfScene);

  SceneNode* animatedGltfAttachmentNode = mainScene->CreateNode("Animated Gltf Attachment");
  Scene* animatedGltfScene = GltfImporter::LoadScene("./res/models/RiggedFigure.glb", "Animated Gltf");
  animatedGltfAttachmentNode->AttachScene(animatedGltfScene);

	mainScene->AddComponent<DebugInspector>();
  mainScene->AddComponent<AnimationSystem>();
}

int main(int, char**) {
	if (!Engine::Setup(InitScene)) {
		spdlog::error("Failed to initialize project!");
		return EXIT_FAILURE;
	}

	spdlog::info("Initialized project.");

	Engine::MainLoop();

	return 0;
}
