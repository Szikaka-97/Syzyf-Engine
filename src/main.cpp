#include "fog/Fog.h"
#include "fog/FogVolume.h"
#include "fog/VolumetricFog.h"
#include "GltfImporter.h"

#include "animation/AnimationSystem.h"
#include "imgui.h"
#include "physics/CharacterController.h"
#include "physics/VirtualCharacterController.h"
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
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

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
#include <Game_Scripts/CameraSettings.h>
#include <Game_Scripts/PlayerController.h>
#include <AiNode.h>
#include "astar/NavigationGrid.h"

#include <vector>

class Mover : public GameObject, public ImGuiDrawable {
private:
	float pitch;
	float rotation;
	bool movementEnabled;
	int mode;
	float movementSpeed = 0.1f;
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
			this->GlobalTransform().Position() += movement * this->movementSpeed;
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

	Mesh* cubeMesh = mainScene->Resources()->Get<Mesh>("./res/models/not_cube.obj");
	Mesh* roomMesh = mainScene->Resources()->Get<Mesh>("./res/models/floor/separateFloors1427.fbx");
	Mesh* dungeon1Mesh = mainScene->Resources()->Get<Mesh>("./res/models/floor/room1.fbx");
	Mesh* dungeon2Mesh = mainScene->Resources()->Get<Mesh>("./res/models/floor/room2.fbx");
	Mesh* schnozMesh = mainScene->Resources()->Get<Mesh>("./res/models/schnoz/schnoz.obj");

	Cubemap* skyCubemap = mainScene->Resources()->Get<Cubemap>("./res/textures/citrus_orchard_road_puresky.hdr", Texture::HDRColorBuffer);
	skyCubemap->SetWrapModeU(TextureWrap::Clamp);
	skyCubemap->SetWrapModeV(TextureWrap::Clamp);
	skyCubemap->SetWrapModeW(TextureWrap::Clamp);

	Texture2D* reflectiveDiffuse = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-albedo.png", Texture::ColorTextureRGB);
	Texture2D* reflectiveNormal = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-Normal-ogl.png", Texture::TechnicalMapXYZ);
	Texture2D* reflectiveARM = mainScene->Resources()->Get<Texture2D>("./res/textures/material_preview/worn-shiny-metal-arm.png", Texture::TechnicalMapXYZ);

	Texture2D* schnozTexture = mainScene->Resources()->Get<Texture2D>("./res/models/schnoz/Diffuse.png", Texture::ColorTextureRGB);

	Viewport* schnozPreview = new Viewport();
	schnozPreview->GetFramebuffer()->CreateColorAttachment(true, false);
	schnozPreview->GetFramebuffer()->CreateDepthAttachment(false, false);
	schnozPreview->SetSize(glm::uvec2(1024, 512));

	Material* reflectiveMat = new Material(pbrProg);
	reflectiveMat->SetValue("albedoMap", reflectiveDiffuse);
	reflectiveMat->SetValue("normalMap", reflectiveNormal);
	reflectiveMat->SetValue("armMap", reflectiveARM);

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	Material* roomMat = new Material(coloredProg);
	roomMat->SetValue("uColor", glm::vec3(1.0, 0.166, 0.234));

	Material* schnozMat = new Material(diffuseTexProg);
	schnozMat->SetValue("uColor", glm::vec3(1, 1, 1));
	schnozMat->SetValue("colorTex", schnozTexture);
	SceneNode* playerNode = mainScene->CreateNode("Player");
	playerNode->GlobalTransform().Position() = glm::vec3(0.0f, 2.0f, 0.0f);
	playerNode->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
	playerNode->AddObject<MeshRenderer>(schnozMesh, reflectiveMat);

	JPH::Ref<JPH::CharacterVirtualSettings> characterSettings = new JPH::CharacterVirtualSettings();
	characterSettings->mShape = new JPH::CapsuleShape(1.0f, 0.5f);
	characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

	auto* virtualCharacter = playerNode->AddObject<Physics::VirtualCharacterController>(characterSettings);
	virtualCharacter->SetPosition(playerNode->GlobalTransform().Position().Value());
	virtualCharacter->Awake();
	//virtualCharacter->SetCollisionLayerAndMask(Physics::Layers::MOVING, { Physics::Layers::NON_MOVING });

	auto mouseMarkerNode = mainScene->CreateNode("Mouse Marker");
	mouseMarkerNode->AddObject<MeshRenderer>(cubeMesh, reflectiveMat);
	mouseMarkerNode->GlobalTransform().Scale() = glm::vec3(0.15f, 0.02f, 0.15f);

	auto* bottleThrower = playerNode->AddObject<ThrowBottle>();
	bottleThrower->SetPoolSize(1);
	bottleThrower->SetResources(cubeMesh, reflectiveMat);

	auto* controller = playerNode->AddObject<PlayerController>(mouseMarkerNode);
	controller->SetBottleThrower(bottleThrower);

	auto cameraNode = mainScene->CreateNode("Camera");
	Camera* camera = cameraNode->AddObject<Camera>(
		Camera::Perspective(25.0f, 16.0f / 9.0f, 0.1f, 200.0f));
	camera->SetAsMainCamera();
	cameraNode->AddObject<CameraSettings>(playerNode);


auto floorNode = mainScene->CreateNode("Floor");
floorNode->AddObject<MeshRenderer>(roomMesh, roomMat);
floorNode->AddObject<Skybox>(skyMat);
floorNode->GlobalTransform().Position() = glm::vec3(0, 0,0);
floorNode->GlobalTransform().Scale() = glm::vec3(10, 10, 10); 
floorNode->GlobalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(90, 0, 0)));

JPH::BodyCreationSettings meshSettings = Physics::Body::Mesh(roomMesh, JPH::EMotionType::Static, Physics::Layers::NON_MOVING);
auto shapeSettings = meshSettings.GetShapeSettings();
if (shapeSettings != nullptr) {
    auto scaledSettings = new JPH::ScaledShapeSettings(shapeSettings, JPH::Vec3(10,10,10));
    auto result = scaledSettings->Create(); 
    if (result.HasError()) {
        spdlog::error("Shape conversion error: {}", result.GetError().c_str());
    } else {
        meshSettings.SetShape(result.Get());
    }
}
meshSettings.mPosition = JPH::RVec3(0, 0,0); 

auto* body = floorNode->AddObject<Physics::Body>(meshSettings);

	
	auto lightNode2 = mainScene->CreateNode("Directional Light");
	lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 4))->SetShadowCasting(true);
	lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
	lightNode2->GlobalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(64.0f, 0.0f, 0.0f)));

	SceneNode* schnozCameraNode = mainScene->CreateNode("Schnoz Camera");
	schnozCameraNode->LocalTransform().Position() = glm::vec3(-56.5, 2.0, -2.0);
	schnozCameraNode->LocalTransform().Rotation() = glm::quat(glm::radians(glm::vec3(5.0f, 85.0f, 0.0f)));

	auto schnozCamera = schnozCameraNode->AddObject<Camera>(Camera::Perspective(40.0f, 16.0f/9.0f, 0.5f, 200.0f));
	schnozCamera->SetAspectRatio(2);
	schnozCamera->SetRenderTarget(schnozPreview);
	schnozCamera->SetLayerMask(uint8_t(5));

	/*floorNode->AddObject<Surface>(gmConstructMesh, 10.0f);
	auto* navGrid = floorNode->AddObject<NavigationGrid>();
	navGrid->Build(floorNode->GetObject<Surface>(), 2.0f, 45.0f);

	SceneNode* w_schnozNode = mainScene->CreateNode("w_schnozNode");
	w_schnozNode->LocalTransform().Position() = glm::vec3(-20,0,-20);
	w_schnozNode->LocalTransform().Scale() = glm::vec3(1,1,1);
	w_schnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);

	JPH::BodyCreationSettings w_schnozShapeSettings = Physics::Body::ConvexHullMesh(schnozMesh, JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
	auto* w_schnozBody = w_schnozNode-> AddObject<Physics::Body>(w_schnozShapeSettings);
	w_schnozBody->SetRestitution(0.0f);
	w_schnozBody->SetFriction(0.5f);
	w_schnozBody->SetLinearDamping(0.1f);
	w_schnozBody->SetCollisionLayerAndMask({ Physics::Layers::MOVING, Physics::Layers::NON_MOVING });

	auto enemyAI = w_schnozNode->AddObject<AiNode>();
	if (enemyAI) {
		enemyAI->SetTarget(playerNode);
		enemyAI->SetProjectileResources(cubeMesh, reflectiveMat); 
		enemyAI->SetAttackCooldown(1.2f);
	}

	glm::vec2 patrolPoints[] = {
		glm::vec2(-20,0),
		glm::vec2(-40,0)
	};

	std::vector<glm::vec2> patrolPointsVec(std::begin(patrolPoints), std::end(patrolPoints));
	w_schnozNode->GetObject<AiNode>()->SetPatrolPoints(patrolPointsVec);*/

	//SceneNode* schnozLightNode = mainScene->CreateNode("Schnoz Light");
	//schnozLightNode->LocalTransform().Position() = glm::vec3(-55.5, 3.0, -2.0);
	//schnozLightNode->AddObject<Light>(Light::PointLight(glm::vec3(1, 1, 1), 5, 5));
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

