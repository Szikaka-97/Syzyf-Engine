#include "ParticleSpawner.h"
#include "fog/Fog.h"
#include "fog/FogVolume.h"
#include "fog/VolumetricFog.h"
#include "GltfImporter.h"

#include "animation/AnimationSystem.h"
#include "imgui.h"
#include "physics/CharacterController.h"
#include "physics/ICollisionReceiver.h"
#include "physics/System.h"
#include "physics/DebugRenderer.h"
#include "physics/Body.h"
#include "physics/Water.h"
#include "physics/LayerMaskFilter.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

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
#include <AiNode.h>

#include <vector>

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

class PhysicsMover : public GameObject, public Physics::ICollisionReceiver, public ImGuiDrawable {
private:
	float pitch;
	float rotation;
	bool movementEnabled;
	int mode;
	float movementSpeed = 10.0f;
	float mouseSensitivity = 1.0f;

  JPH::Character* character = nullptr;
  SceneNode* heldItem = nullptr;
  JPH::BodyID floorId;
  SceneNode* cameraNode = nullptr;
public:
	PhysicsMover() {
		this->pitch = 0;
		this->rotation = 0;
		this->mode = 0;

    // will crash if added before character remove tis
    this->character = this->GetObject<Physics::CharacterController>()->GetCharacter();
    // this->cameraNode = this->GetNode()->GetObjectInChildren<Camera>()->GetNode();
	}

	void Update() {
    if (cameraNode == nullptr) {
      this->cameraNode = this->GetNode()->GetObjectInChildren<Camera>()->GetNode();
      if (cameraNode == nullptr) return;
    }
    if (this->floorId.IsInvalid()) {
      // :frog:
      this->floorId = this->GetScene()->FindObjectsOfType<Skybox>().front()->GetNode()->GetObject<Physics::Body>()->GetBodyID();
      if (this->floorId.IsInvalid()) return;
    }

    JPH::Vec3 position = this->character->GetPosition();
    this->GlobalTransform().Position() = {position.GetX(), position.GetY(), position.GetZ()};

		if (movementEnabled) {
			glm::vec3 movement = glm::zero<glm::vec3>();
			glm::quat rotation = glm::identity<glm::quat>();

			glm::vec3 right = this->cameraNode->GlobalTransform().Right();
			glm::vec3 up = glm::vec3(0, 1, 0);
			glm::vec3 forward = mode == 0 ? glm::cross(right, up) : this->cameraNode->GlobalTransform().Forward();
      bool jump = false;

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
      if (GetScene()->Input()->KeyPressed(Key::Space)) {
        jump = true;
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
			this->cameraNode->LocalTransform().Rotation() = glm::angleAxis(
				glm::radians(this->rotation), glm::vec3(0, 1, 0)
			) * glm::angleAxis(glm::radians(this->pitch), glm::vec3(1, 0, 0));

      JPH::Vec3 jphMovement = JPH::Vec3(movement.x, 0.0f, movement.z);

      JPH::Character::EGroundState groundState = this->character->GetGroundState();
      if (groundState == JPH::Character::EGroundState::OnSteepGround
          || groundState == JPH::Character::EGroundState::NotSupported) {
       // spdlog::info("PhysicsMover: Character on steep ground");
        JPH::Vec3 normal = this->character->GetGroundNormal();
        normal.SetY(0.0f);
        float dot = normal.Dot(jphMovement);
        if (dot < 0.0f) {
          jphMovement -= (dot * normal) / normal.LengthSq(); 
        }
      }

      if (this->character->IsSupported()) {
        JPH::Vec3 currentVelocity = this->character->GetLinearVelocity();
        JPH::Vec3 desiredVelocity = this->movementSpeed * jphMovement;

        if (!desiredVelocity.IsNearZero() || currentVelocity.GetY() < 0.0f || !this->character->IsSupported()) {
          desiredVelocity.SetY(currentVelocity.GetY());
        } 
        JPH::Vec3 newVelocity = 0.75f * currentVelocity + 0.25f * desiredVelocity;

        if (jump && groundState == JPH::Character::EGroundState::OnGround) {
          newVelocity += JPH::Vec3(0, this->movementSpeed * 0.25, 0);
        }

        this->character->SetLinearVelocity(newVelocity);
      }
    }

    if (GetScene()->Input()->ButtonUp(MouseButton::Left)) {
      if (heldItem) {
        if (auto* body = heldItem->GetObject<Physics::Body>()) {
          body->SetPosition(heldItem->GlobalTransform().Position());
          body->OnEnable();
          this->heldItem = nullptr;
        }
      }
    }

    if (GetScene()->Input()->ButtonDown(MouseButton::Left)) {
      auto* physics = this->GetScene()->GetComponent<Physics::System>();
     
      JPH::RVec3 origin = {
        this->cameraNode->GlobalTransform().Position().x,
        this->cameraNode->GlobalTransform().Position().y,
        this->cameraNode->GlobalTransform().Position().z
      };

      JPH::Vec3 direction = JPH::Vec3(
        this->cameraNode->GlobalTransform().Forward().x,
        this->cameraNode->GlobalTransform().Forward().y,
        this->cameraNode->GlobalTransform().Forward().z
      ) * 100.0f;

      Physics::LayerMaskFilter bodyFilter({1}, false);

      bodyFilter.IgnoreBody(this->character->GetBodyID());
      bodyFilter.IgnoreBody(this->floorId);


      SceneNode* result = physics->CastRay(
        this->cameraNode->GlobalTransform().Position(),
        this->cameraNode->GlobalTransform().Forward() * 100.0f,
        {},
        {},
        bodyFilter
      );

      if (result) {
      //  spdlog::info("Raycast hit");

     //   spdlog::info("Hit node: {}", result->GetName());
        if (auto* object = result->GetObject<Physics::Body>()) {
          object->ApplyImpulse(this->cameraNode->GlobalTransform().Forward() * 100.0f);
          if (result->GetName() == "Physics Schnoz") {
            heldItem = result;
            object->OnDisable();
          }
        //  spdlog::info("Applied impulse");
        } else {
         // spdlog::info("Not a physics object");
        }
      }
    }
      if (heldItem) {
        heldItem->GlobalTransform().Position() = this->cameraNode->GlobalTransform().Position() + this->cameraNode->GlobalTransform().Forward() * 2.0f;
      }

    if (GetScene()->Input()->ButtonDown(MouseButton::Right)) {
      auto* physics = this->GetScene()->GetComponent<Physics::System>();

      JPH::Vec3 direction = JPH::Vec3(
        this->cameraNode->GlobalTransform().Forward().x,
        this->cameraNode->GlobalTransform().Forward().y,
        this->cameraNode->GlobalTransform().Forward().z
      ) * 100.0f;

      JPH::ShapeRefC shape = new JPH::SphereShape(0.5f);

      std::vector<SceneNode*> results = physics->CastShape(
        this->cameraNode->GlobalTransform().Position(),
        this->cameraNode->GlobalTransform().Forward() * 100.0f,
        shape,
        {},
        {},
        JPH::IgnoreSingleBodyFilter(this->character->GetBodyID())
      );

      if (!results.empty()) {
       // spdlog::info("Shape cast hit {} objects", results.size());

        for (SceneNode* result : results) {
          if (result) {
            spdlog::info("Hit: {}", result->GetName());
          }
        }
      } else {
       // spdlog::info("ShapeCast hit nothing");
      }
    }

		if (GetScene()->Input()->KeyDown(Key::Escape)) {
			this->movementEnabled = !this->movementEnabled;

			GetScene()->Input()->SetMouseLocked(this->movementEnabled);
		}
	}

  virtual void OnCollisionEnter(SceneNode* node) {
   // spdlog::info("PhysicsMover collided with: {}", node->GetName());
  }

	virtual void DrawImGui() {
		const char* modes[] { "Walking", "Freecam", };

		ImGui::Combo("Movement type", &this->mode, modes, 2);

		ImGui::InputFloat("Movement speed", &this->movementSpeed);
		ImGui::InputFloat("Mouse sensitivity", &this->mouseSensitivity);
	}

  virtual void OnCollisionExit(SceneNode* node) {}
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

class Tornado : public GameObject, public ImGuiDrawable {
private:
    Mesh* tornadoMesh;
    std::unique_ptr<ShaderProgram> tornadoShader;
    std::unique_ptr<Material> tornadoMaterial;
    SceneNode* tornadoNode;

    Mesh* particlesMesh;
    std::unique_ptr<ShaderProgram> particlesShader;
    std::unique_ptr<Material> particlesBottomMaterial;
    std::unique_ptr<Material> particlesTopMaterial;
    SceneNode* particlesBottom;
    SceneNode* particleTop;

    float rotationSpeed = 10.0f;
    float opacity = 0.7f;
    float intensity = 5.5f;
    glm::vec2 uvSpeed = { 2.0f, 1.0f };
public:
	Tornado() {
        this->tornadoMesh = this->GetScene()->Resources()->Get<Mesh>("./res/models/tornado/tornado.obj");
        this->tornadoShader.reset(ShaderProgram::Build()
            .WithVertexShader(
                GetScene()->Resources()->Get<VertexShader>("./res/shaders/lit.vert")
            ).WithPixelShader(
                GetScene()->Resources()->Get<PixelShader>("./res/shaders/tornado.frag")
            ).Link());
        this->tornadoShader->SetCastsShadows(false);
        this->tornadoShader->SetTransparent(true);
        this->tornadoMaterial = std::make_unique<Material>(
            this->tornadoShader.get()
        );
        this->tornadoMaterial->SetValue("uOpacity", this->opacity);
        this->tornadoMaterial->SetValue("uSpeed", this->uvSpeed);
        this->tornadoMaterial->SetValue("uIntensity", this->intensity);

        this->tornadoMaterial->SetValue(
            "uNoiseTexture",
            this->GetScene()->Resources()->Get<Texture2D>("./res/textures/noise/T_Noise_HU85k.png", Texture::TechnicalMapXYZ)
        );
        this->tornadoMaterial->SetValue(
            "uColorGradientTexture",
            this->GetScene()->Resources()->Get<Texture2D>("./res/textures/inkpink-alpha.png", Texture::ColorTextureRGBA)
        );
        this->tornadoMaterial->SetValue(
            "uSubtractionTexture",
            this->GetScene()->Resources()->Get<Texture2D>("./res/textures/subtraction.png", Texture::TechnicalMapXYZ)
        );
        
        this->tornadoNode = this->GetScene()->CreateNode(this->GetNode(), "Tornado");
        this->tornadoNode->AddObject<MeshRenderer>(this->tornadoMesh, this->tornadoMaterial.get());

        this->particlesBottom = this->GetScene()->CreateNode(this->GetNode(), "Tornado Particles Bottom");
        this->particlesMesh = this->GetScene()->Resources()->Get<Mesh>("./res/models/tornado/tornado_base.obj");
        this->particlesShader.reset(ShaderProgram::Build()
            .WithVertexShader(
                this->GetScene()->Resources()->Get<VertexShader>("./res/shaders/particles/particles.vert")
            ).WithPixelShader(
                this->GetScene()->Resources()->Get<PixelShader>("./res/shaders/particles/tornado_base.frag")
            ).Link()
        );
        this->particlesShader->SetCastsShadows(false);
        this->particlesShader->SetIgnoresDepthPrepass(true);
        this->particlesShader->SetTransparent(true);

        this->particlesBottomMaterial = std::make_unique<Material>(this->particlesShader.get());
        this->particlesTopMaterial = std::make_unique<Material>(this->particlesShader.get());

        this->particlesBottomMaterial->SetValue(
            "colorTex",
            this->GetScene()->Resources()->Get<Texture2D>("./res/textures/noise/T_FirePanningCyl45.png", Texture::TechnicalMapXYZ)
        );

        Texture2D* colorRampTex = this->GetScene()->Resources()->Get<Texture2D>("./res/textures/inkpink-32x.png", Texture::ColorTextureRGB);
        this->particlesBottomMaterial->SetValue("colorRamp", colorRampTex);

        ParticleSpawnerSettings particleSettings = {
            .maxParticles = 8,
            .emissionShapeExtents = glm::vec3(0.01f),

            .minVelocity = { 0.0f, 0.0f, 0.0f },
            .maxVelocity = { 0.0f, 0.2f, 0.0f },

            .minInitialAngle = glm::radians(-180.0f),
            .maxInitialAngle = glm::radians(180.0f),
            .minAngularVelocity = -4.0f,
            .maxAngularVelocity = 8.0f,
            .rotateY = true,

            .enableLifetime = true,
            .minLifetime = 2.0f,
            .maxLifetime = 3.0f,

            .lifetimeFadeMode = FadeMode::Alpha,
            .lifetimeFadeIn = { 0.0f, 0.1f },
            .lifetimeFadeOut = { 0.9f, 1.0f },

            .minScale = 1.5f,
            .maxScale = 1.8f,

            .wrapAround = false,
            .continuous = true,
            .useColorRamp = true,
        };

        this->particlesBottom->AddObject<ParticleSpawner>(
            this->particlesMesh,
            std::move(particlesBottomMaterial),
            particleSettings 
        );

        this->particleTop = this->GetScene()->CreateNode(this->GetNode(), "Tornado Particles Top");
        this->particlesTopMaterial->SetValue(
            "colorTex",
            this->GetScene()->Resources()->Get<Texture2D>("./res/textures/noise/T_FirePanningCyl45.png", Texture::TechnicalMapXYZ)
        );
        this->particlesTopMaterial->SetValue("colorRamp", colorRampTex);

        particleSettings.minScale = 3.5f;
        particleSettings.maxScale = 4.0f;
        this->particleTop->AddObject<ParticleSpawner>(
            this->particlesMesh,
            std::move(particlesTopMaterial),
            particleSettings
        );
        this->particleTop->LocalTransform().Position() = { 0.0f, 4.8f, 0.0f };
    }

	void Update() {
		glm::quat rotation = glm::angleAxis(glm::radians(this->rotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));

		this->tornadoNode->LocalTransform().Rotation() *= rotation;
	}

    void DrawImGui() {
        ImGui::SliderFloat("Rotation Speed", &this->rotationSpeed, 0.0f, 50.0f);
        if (ImGui::SliderFloat("Opacity", &this->opacity, 0.0f, 1.0f)) {
            this->tornadoMaterial->SetValue("uOpacity", this->opacity);
        }
        if (ImGui::InputFloat2("UV Speed", &this->uvSpeed[0])) {
            this->tornadoMaterial->SetValue("uSpeed", this->uvSpeed);
        }
        if (ImGui::SliderFloat("Intensity", &this->intensity, 0.0f, 10.0f)) {
            this->tornadoMaterial->SetValue("uIntensity", this->intensity);
        }
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
  mainScene->AddComponent<Physics::System>();
  mainScene->AddComponent<Physics::DebugRenderer>();

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

	ShaderProgram* pbrRefractProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/pbr refract.frag")
	).Link();

	ShaderProgram* transparentProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/lit.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/transparent.frag")
	).Link();
	transparentProg->SetTransparent(true);

	 Mesh* gmConstructMesh = mainScene->Resources()->Get<Mesh>("./res/models/construct/construct.obj", true);
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

	Material* shinyMat = new Material(pbrRefractProg);
	shinyMat->SetValue("albedoMap", reflectiveDiffuse);
	shinyMat->SetValue("normalMap", reflectiveNormal);
	shinyMat->SetValue("armMap", reflectiveARM);

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

	Material* blueTransparentMat = new Material(transparentProg);
	blueTransparentMat->SetValue("uColor", glm::vec4(0.5, 0.5, 1.0, 0.6));

	 //auto constructNode = mainScene->CreateNode("gm_construct");
	 //constructNode->AddObject<MeshRenderer>(gmConstructMesh, gmConstructMesh->GetDefaultMaterials());
	 // constructNode->AddObject<Physics::Body>(Physics::Body::Mesh(gmConstructMesh, JPH::EMotionType::Static, Physics::Layers::NON_MOVING));

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

  SceneNode* playerNode = mainScene->CreateNode("Player");
  playerNode->GlobalTransform().Position() = glm::vec3(2.0f, 30.0f, -10.0f);
  JPH::Ref<JPH::CharacterSettings> characterSettings = new JPH::CharacterSettings();
  characterSettings->mShape = new JPH::CapsuleShape(1.0f, 0.5f);
  characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
  characterSettings->mFriction = 0.5f;
  characterSettings->mLayer = Physics::Layers::MOVING;
  playerNode->AddObject<Physics::CharacterController>(characterSettings);
  playerNode->AddObject<PhysicsMover>();

    Texture2D* smokeTex = mainScene->Resources()->Get<Texture2D>("./res/textures/smoke_08.png", Texture::ColorTextureRGBA);
    Mesh* quadMesh = mainScene->Resources()->Get<Mesh>("./res/models/quad.obj");
	ShaderProgram* particleProg = ShaderProgram::Build().WithVertexShader(
		mainScene->Resources()->Get<VertexShader>("./res/shaders/particles/particles.vert")
	).WithPixelShader(
		mainScene->Resources()->Get<PixelShader>("./res/shaders/particles/particles.frag")
	).Link();
    particleProg->SetCastsShadows(false);
    particleProg->SetIgnoresDepthPrepass(false);
    particleProg->SetTransparent(true);
    std::unique_ptr<Material> schnozInstancedMat = std::make_unique<Material>(particleProg);
	schnozInstancedMat->SetValue("colorTex", smokeTex);
	   // playerNode->AddObject<ParticleSpawner>(
	   //     quadMesh,
	   //     std::move(schnozInstancedMat),
	   //     ParticleSpawnerSettings {
	   //         .maxParticles = 1024,
	   //         .areaExtents = glm::vec3(150.0f),
	   //         .emissionShapeExtents = glm::vec3(150.0f),
	   //
	   //         .maxLifetime = 1000.0f,
	   //
	   //         .minScale = 15.0f,
	   //         .maxScale = 30.0f,
	   //
	   //         .proximityFadeMode = FadeMode::Alpha,
	   //         .proximityFadeMin = 3.0f,
	   //         .proximityFadeMax = 8.0f,
	   //
	   //
	   //         .distanceFadeMode = FadeMode::Alpha,
	   //         .distanceFadeMin = 130.0f,
	   //         .distanceFadeMax = 140.0f,
	   //
	   //            .enableDepthFade = true,
	   //            .depthFadeDistance = 3.0f,
	   //
	   //         .billboardMode = BillboardMode::Z,
	   //
	   //         .wrapAround = true,
	   //         .continuous = false,
	   //     }
	   // );

    std::unique_ptr<Material> schnozInstancedMat2 = std::make_unique<Material>(particleProg);
	Texture2D* smoke2Tex = mainScene->Resources()->Get<Texture2D>("./res/textures/T_smoke_b7.png", Texture::ColorTextureRGBA);
	schnozInstancedMat2->SetValue("colorTex", smoke2Tex);
    Texture2D* colorRampTex = mainScene->Resources()->Get<Texture2D>("./res/textures/inkpink-32x.png", Texture::ColorTextureRGB);
    schnozInstancedMat2->SetValue("colorRamp", colorRampTex);
    SceneNode* smokeParticlesNode = mainScene->CreateNode("Smoke Particles");
    smokeParticlesNode->AddObject<ParticleSpawner>(
        quadMesh,
        std::move(schnozInstancedMat2),
        ParticleSpawnerSettings {
            .maxParticles = 32,
            .areaExtents = { 2.0f, 8.0f, 2.0f },
            .emissionShapeExtents = { 0.2f, 0.2f, 0.2f },
            .minVelocity = { 0.0f, 2.0f, 0.0f },
            .maxVelocity = { 0.0f, 2.0f, 0.0f },

            .minInitialAngle = glm::radians(-180.0f),
            .maxInitialAngle = glm::radians(180.0f),
            .minAngularVelocity = -1.5f,
            .maxAngularVelocity = 1.5f,

            .minLifetime = 3.0f,
            .maxLifetime = 4.0f,

            .proximityFadeMode = FadeMode::Alpha,
            .proximityFadeMin = 0.5f,
            .proximityFadeMax = 3.0f,

            .lifetimeFadeMode = FadeMode::Alpha,
            .lifetimeFadeIn = { 0.0f, 0.1f },
            .lifetimeFadeOut = { 0.9f, 1.0f },

            .billboardMode = BillboardMode::Enabled,

            .wrapAround = false,
            .continuous = true,
            .useColorRamp = true,
        }
    );
    smokeParticlesNode->GlobalTransform().Position() = { 0.0f, 2.0f, 0.0f };

	auto cameraNode = mainScene->CreateNode(playerNode, "Camera");
	Camera* camera = cameraNode->AddObject<Camera>(Camera::Perspective(40.0f, 16.0f/9.0f, 0.5f, 200.0f));
	camera->GlobalTransform().Position() = glm::vec3(2.0f, 30.0f, -10.0f);

  //auto* floorMesh = mainScene->Resources()->Get<Mesh>("./res/models/floor/floor.obj", true);
	auto floorNode = mainScene->CreateNode("Floor");
    floorNode->AddObject<MeshRenderer>(gmConstructMesh, gmConstructMesh->GetDefaultMaterials());
	floorNode->AddObject<Skybox>(skyMat); 
	floorNode->AddObject<Physics::Body>(Physics::Body::Mesh(gmConstructMesh, JPH::EMotionType::Static, Physics::Layers::NON_MOVING));
	floorNode->AddObject<Surface>(gmConstructMesh, 1.0f);

	auto lightNode = mainScene->CreateNode("Point Light");
	lightNode->AddObject<Light>(Light::PointLight({1, 1, 1}, 10, 2))->SetShadowCasting(false);
	lightNode->GlobalTransform().Position() = {-1, 2.2f, 0};

	auto lightNode2 = mainScene->CreateNode("Directional Light");
	lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 4))->SetShadowCasting(true);
	lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
	lightNode2->GlobalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(64.0f, 0.0f, 0.0f)));

	auto envProbe = mainScene->CreateNode(cubeNode, "Reflection Probe");
	envProbe->AddObject<ReflectionProbe>();

	auto envProbe2 = mainScene->CreateNode("Reflection Probe");
	envProbe2->AddObject<ReflectionProbe>();
	envProbe2->GlobalTransform().Position() = {-10.0f, 1.5f, 0.6f};

	auto envProbe3 = mainScene->CreateNode("Reflection Probe");
	envProbe3->AddObject<ReflectionProbe>();
	envProbe3->GlobalTransform().Position() = {-29.0f, 1.5f, 0.6f};

	auto envProbe4 = mainScene->CreateNode(shinyCubeNode, "Reflection Probe");
	envProbe4->AddObject<ReflectionProbe>();
	
	// auto starsAttachmentNode = mainScene->CreateNode("Stars Scene Attachment");
	//
	// auto starsScene = new Scene();
	//
	// auto starsNode = starsScene->CreateNode("Stars");
	// starsNode->AddObject<Stars>(1000);
	// starsNode->GlobalTransform().Position() = {-15.0f, 5.5f, -105.0f};
	//
	// starsAttachmentNode->AttachScene(starsScene);

	SceneNode* tvNode = mainScene->CreateNode("TV");
	tvNode->LocalTransform().Scale() = glm::vec3(1.5, 1.5, 1.5);
	tvNode->LocalTransform().Position() = glm::vec3(3, -5, -2);
	tvNode->LocalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(-90.0f, 20.0f, 0.0f)));

	auto tvRenderer = tvNode->AddObject<MeshRenderer>(tvMesh, nullptr);
	tvRenderer->SetMaterial(tvMatStand, 0);
	tvRenderer->SetMaterial(screenMat, 1);
	tvRenderer->SetMaterial(tvMatStand, 2);
	tvRenderer->SetMaterial(tvMatStand, 3);

	SceneNode* fogVolume = mainScene->CreateNode("Fog Volume");
	FogVolume* fogVolumeObject = fogVolume->AddObject<FogVolume>();
  fogVolumeObject->stepSize = 0.06f;
  fogVolumeObject->scatteringDensity = 0.042f;
  fogVolumeObject->absorptionDensity = 0.0f;
  fogVolumeObject->k = 0.005f;
	fogVolume->GlobalTransform().Position() = { -28.0f, 1.5f, 0.0f };
	fogVolume->GlobalTransform().Scale() = { 20.0f, 12.0f, 20.0f };

  SceneNode* fogVolume2 = mainScene->CreateNode("Fog Volume 2");
  FogVolume* fogVolume2Object = fogVolume2->AddObject<FogVolume>();
  fogVolume2Object->scatteringColor = { 173, 0, 255 };
  fogVolume2Object->stepSize = 0.03f;
  fogVolume2Object->scatteringDensity = 2.0f;
  fogVolume2Object->absorptionDensity = 0.0f;
  fogVolume2->GlobalTransform().Position() = { 0.0f, 0.6f, 3.0f };

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

	SceneNode* w_schnozNode = mainScene->CreateNode("w_schnozNode");
	w_schnozNode->LocalTransform().Position() = glm::vec3(-20,0,-20);
	schnozNode->LocalTransform().Scale() = glm::vec3(1,1,1);
	w_schnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);

	

	JPH::BodyCreationSettings w_schnozShapeSettings = Physics::Body::ConvexHullMesh(schnozMesh, JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
	auto* w_schnozBody = w_schnozNode-> AddObject<Physics::Body>(w_schnozShapeSettings);
	w_schnozBody->SetRestitution(0.0f);
	w_schnozBody->SetFriction(0.5f);
	w_schnozBody->SetLinearDamping(0.1f);
	w_schnozBody->Awake();
	w_schnozBody->SetCollisionLayerAndMask({ 0 });

	auto enemyAI = w_schnozNode->AddObject<AiNode>();
	if (enemyAI) {
		enemyAI->SetTarget(cameraNode);
	}

	glm::vec2 patrolPoints[] = {
		glm::vec2(-20,0),
		glm::vec2(-40,0)
	};

	/*auto aiNode = w_schnozNode->GetObject<AiNode>();
	if (aiNode) {
		aiNode->SetPatrolPoints(patrolPointsVec);
	}*/


	std::vector<glm::vec2> patrolPointsVec(std::begin(patrolPoints), std::end(patrolPoints));
	w_schnozNode->GetObject<AiNode>()->SetPatrolPoints(patrolPointsVec);

	SceneNode* schnozLightNode = mainScene->CreateNode("Schnoz Light");
	schnozLightNode->LocalTransform().Position() = glm::vec3(-55.5, 3.0, -2.0);
	schnozLightNode->AddObject<Light>(Light::PointLight(glm::vec3(1, 1, 1), 5, 5));


  // cameraNode->AddObject<VolumetricFog>();
  for (int i = 0; i < 50; ++i) {
    SceneNode* physicsSchnozNode = mainScene->CreateNode("Physics Schnoz");
    physicsSchnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);
    physicsSchnozNode->GlobalTransform().Position() = { 2.0f + i, 10.0f + i * 2.0f, 0.0f - i};
    physicsSchnozNode->GlobalTransform().Scale() = glm::vec3(0.25f);
    JPH::BodyCreationSettings schnozShapeSettings = Physics::Body::ConvexHullMesh(schnozMesh, JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
    auto* schnozBody = physicsSchnozNode->AddObject<Physics::Body>(schnozShapeSettings);
    
    schnozBody->SetCollisionLayerAndMask({0});
  }

	cameraNode->AddObject<Bloom>();
	cameraNode->AddObject<Tonemapper>()->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);
    // cameraNode->AddObject<Fog>();

  Mesh* waterMesh = mainScene->Resources()->Get<Mesh>("./res/models/water.obj");
  SceneNode* water = mainScene->CreateNode("Water");
  water->AddObject<MeshRenderer>(waterMesh, blueTransparentMat);
  water->AddObject<Physics::Water>();
  water->GlobalTransform().Position() = {
    4.0f, 0.0f, -8.0f
  };
  auto* waterBody = water->AddObject<Physics::Body>(Physics::Body::ConvexHullMesh(
    waterMesh,
    JPH::EMotionType::Static,
    Physics::Layers::NON_MOVING
  ));
  waterBody->SetIsSensor(true);
  waterBody->SetCollisionLayerAndMask({1});

  Scene* animatedGltfScene = GltfImporter::LoadScene("./res/models/jake_tangents.glb", "Animated Gltf");
  mainScene->GetRootNode()->AttachScene(animatedGltfScene);

	mainScene->AddComponent<DebugInspector>();
  mainScene->AddComponent<AnimationSystem>();

  SceneNode* tornadoNode = mainScene->CreateNode("Tornado Node");
  tornadoNode->AddObject<Tornado>();
    tornadoNode->GlobalTransform().Position() = { 5.0f, 0.0f, 0.0f };
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

