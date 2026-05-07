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
#include "Player.h"
#include <AiSimplified.h>
#include <animation/AnimationSystem.h>
#include <animation/AnimationSystem.h>
#include <animation/SkeletonComponent.h>
#include <animation/SkeletonSystem.h>

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
      ).node;

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
			"./res/shaders/star.vert"
		).WithGeometryShader(
			"./res/shaders/star.geom"
		).WithPixelShader(
			"./res/shaders/star.frag"
		).Link();
		//starProgram->SetIgnoresDepthPrepass(true);
		//starProgram->SetCastsShadows(false);

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

//void SingleWall(SceneNode* node, glm::vec3 pos, glm::vec3 wallSize,Mesh* cubeMesh,Material* roomMat, std::string name ) {
//	JPH::BodyCreationSettings playerRoomSettings = Physics::Body::ConvexHullMesh(cubeMesh, JPH::EMotionType::Static, Physics::Layers::NON_MOVING);
//	auto wallNode = node->GetScene()->CreateNode(node->GetName() + " Wall"+ name);
//	wallNode->AddObject<MeshRenderer>(cubeMesh, roomMat);
//	wallNode->GlobalTransform().Position() = (pos);
//	wallNode->GlobalTransform().Scale() = wallSize;
//	auto* wallBody = wallNode->AddObject<Physics::Body>(playerRoomSettings);
//	wallNode->SetParent(node);
//}
//
///// <summary>
///// got confused with directions, actual set of params is WEST EAST NORT SOUTH 
///// </summary>
//void MakeWalls(Mesh* cubeMesh, Material* roomMat, SceneNode* node, bool a, bool b, bool c, bool d) {
//    glm::vec3 pos = node->GlobalTransform().Position();
//    glm::vec3 size = node->GlobalTransform().Scale();   
//    float wallThick = 0.2f;
//    float wallHeight = size.y + 4.0f;
//    
//    if (d) {
//        glm::vec3 wallSize = glm::vec3(wallThick, wallHeight, size.z);
//        glm::vec3 wallPos = glm::vec3(
//            pos.x - size.x/2.0f - wallThick/2.0f,
//            pos.y + size.y/2.0f + wallHeight/2.0f,
//            pos.z
//        );
//        SingleWall(node, wallPos, wallSize, cubeMesh, roomMat, "d");
//    }
//    if (c) {
//        glm::vec3 wallSize = glm::vec3(wallThick, wallHeight, size.z);
//        glm::vec3 wallPos = glm::vec3(
//            pos.x + size.x/2.0f + wallThick/2.0f,
//            pos.y + size.y/2.0f + wallHeight/2.0f,
//            pos.z
//        );
//        SingleWall(node, wallPos, wallSize, cubeMesh, roomMat, "c");
//    }
//    if (b) {
//        glm::vec3 wallSize = glm::vec3(size.x, wallHeight, wallThick);
//        glm::vec3 wallPos = glm::vec3(
//            pos.x,
//            pos.y + size.y/2.0f + wallHeight/2.0f,
//            pos.z + size.z/2.0f + wallThick/2.0f
//        );
//        SingleWall(node, wallPos, wallSize, cubeMesh, roomMat, "b");
//    }
//    if (a) {
//        glm::vec3 wallSize = glm::vec3(size.x, wallHeight, wallThick);
//        glm::vec3 wallPos = glm::vec3(
//            pos.x,
//            pos.y + size.y/2.0f + wallHeight/2.0f,
//            pos.z - size.z/2.0f - wallThick/2.0f
//        );
//        SingleWall(node, wallPos, wallSize, cubeMesh, roomMat, "a");
//    }
//}
//
//void MakeRooms(Mesh* cubeMesh,Material* roomMat, Scene* mainScene, Material* skyMat) {
//	JPH::BodyCreationSettings playerRoomSettings = Physics::Body::ConvexHullMesh(cubeMesh, JPH::EMotionType::Static, Physics::Layers::NON_MOVING);
//
//	auto playerRoomNode = mainScene->CreateNode("Player Room");
//    playerRoomNode->AddObject<MeshRenderer>(cubeMesh, roomMat);
//    playerRoomNode->AddObject<Skybox>(skyMat);
//    playerRoomNode->GlobalTransform().Scale() = glm::vec3(10.0f, 0.2f, 10.0f);
//    playerRoomNode->GlobalTransform().Position() = glm::vec3(0.0f, -0.5f, 0.0f);
//    auto* playerRoomBody = playerRoomNode->AddObject<Physics::Body>(playerRoomSettings);
//	MakeWalls(cubeMesh, roomMat, playerRoomNode,true,true,false, true);
//	playerRoomNode->AddObject<Surface>(cubeMesh);
//	playerRoomNode->GetObject<Surface>()->SetID(0);
//
//	auto enemyRoomNode = mainScene->CreateNode("Enemy Room");
//	
//	enemyRoomNode->GlobalTransform().Scale() = glm::vec3(10.0f, 0.2f, 10.0f);
//	enemyRoomNode->GlobalTransform().Position() = glm::vec3(10.5f, -0.5f, 0.0f);
//	enemyRoomNode->AddObject<MeshRenderer>(cubeMesh, roomMat);
//	///
//	enemyRoomNode->AddObject<Surface>(cubeMesh);
//	enemyRoomNode->GetObject<Surface>()->SetID(1);
//	//enemyRoomNode->GetObject<Surface>()->SetRoomID(1);
//	//auto* navGrid = enemyRoomNode->AddObject<NavigationGrid>();
//	//navGrid->Build(enemyRoomNode->GetObject<Surface>(), 2.0f, 45.0f);
//	///
//	auto* enemyRoomBody = enemyRoomNode->AddObject<Physics::Body>(playerRoomSettings);
//	MakeWalls(cubeMesh, roomMat, enemyRoomNode,true,false,true, false);
//
//	auto enemyRoomNode2 = mainScene->CreateNode("Enemy Room 2");
//	enemyRoomNode2->AddObject<MeshRenderer>(cubeMesh, roomMat);
//	enemyRoomNode2->GlobalTransform().Scale() = glm::vec3(14.0f, 0.2f, 18.0f);
//	enemyRoomNode2->GlobalTransform().Position() = glm::vec3(10.5f, -0.5f, 14.5f);
//	auto* enemyRoomBody2 = enemyRoomNode2->AddObject<Physics::Body>(playerRoomSettings);
//  	MakeWalls(cubeMesh, roomMat, enemyRoomNode2,false,true,true, true);
//}
//
//
//void AddEnemies(Mesh* enemyMesh, Material* enemyMat, Scene* mainScene, SceneNode* target, Mesh* cubeMesh, Material* reflectiveMat) {
//    JPH::BodyCreationSettings enemyShapeSettings = Physics::Body::ConvexHullMesh(enemyMesh, JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
//	    auto enemyRoom = mainScene->FindNode("Enemy Room");
//   auto* surface = enemyRoom->GetObject<Surface>();
//	target->AddObject<Player>();
//
//    SceneNode* enemy1 = mainScene->CreateNode("Enemy 1");
//    enemy1->AddObject<MeshRenderer>(enemyMesh, enemyMat);
//    enemy1->GlobalTransform().Position() = glm::vec3(10.5f, 0.0f, 2.0f);
//    enemy1->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
//
//    auto* enemyBody1 = enemy1->AddObject<Physics::Body>(enemyShapeSettings);
//	auto* enemyAi1 = enemy1->AddObject<AiNode>();
//
//
//            enemyAi1->SetSurface(surface);
//			surface->AddEnemy(enemyAi1);
//
//    enemyAi1->SetTarget(target);
//    enemyAi1->SetProjectileResources(cubeMesh, reflectiveMat);
//    enemyAi1->SetAttackCooldown(1.2f);
//	enemyAi1->SetRoomID(1);
//	if (auto* animComp = enemyAi1->GetObject<AnimationComponent>()) {
//    enemyAi1->SetAttackAnimation(animComp);
//
//}
//	auto animatedScene = GltfImporter::LoadScene("./res/models/jake_tangents.glb", "Animated Gltf");
//AnimationComponent* animComponents = animatedScene->GetRootNode()->GetObjectInChildren<AnimationComponent>();
//if (animComponents && !animComponents->animations.empty()) {
// 
//    enemyAi1->SetAttackAnimation(animComponents);   // użyj pierwszego znalezionego
//}
//	//SceneNode* enemy2 = mainScene->CreateNode("Enemy 2");
//	//enemy2->AddObject<MeshRenderer>(enemyMesh, enemyMat);
//	//enemy2->GlobalTransform().Position() = glm::vec3(10.5f, 0.0f, 3.0f);
//	//enemy2->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
//	//auto* enemyBody2 = enemy2->AddObject<Physics::Body>(enemyShapeSettings);
//	//auto* enemyAi2 = enemy2->AddObject<AiSimplified>();
//	//enemyAi2->SetSurface(surface);
//	////surface->AddEnemy(enemyAi2);
//	//glm::vec3 targetPos = glm::vec3(12.0f, 0.0f, -2.0f);
//	//enemyAi2->SetTarget(targetPos);
//	// /*enemyAi2->SetProjectileResources(cubeMesh, reflectiveMat);
//	//enemyAi2->SetAttackCooldown(1.0f);
//	//enemyAi2->SetRoomID(1);*/
//
//	//Scene* animatedGltfScene = GltfImporter::LoadScene("./res/models/jake_tangents.glb", "Animated Gltf");
//	 //mainScene->GetRootNode()->AttachScene(animatedGltfScene);
//
//}

void InitScene(Scene* mainScene) {
  mainScene->AddComponent<Physics::System>();
 // mainScene->AddComponent<Physics::DebugRenderer>();

	ShaderProgram* skyProg = ShaderProgram::Build().WithVertexShader(
		"./res/shaders/skybox.vert"
	).WithPixelShader(
		"./res/shaders/skybox.frag"
	).Link();

	ShaderProgram* coloredProg = ShaderProgram::Build().WithVertexShader(
		"./res/shaders/lit.vert"
	).WithPixelShader(
		"./res/shaders/lambert color.frag"
	).Link();

	ShaderProgram* diffuseTexProg = ShaderProgram::Build().WithVertexShader(
		"./res/shaders/lit.vert"
	).WithPixelShader(
		"./res/shaders/lambert.frag"
	).Link();

	ShaderProgram* pbrProg = ShaderProgram::Build().WithVertexShader(
		"./res/shaders/lit.vert"
	).WithPixelShader(
		"./res/shaders/pbr.frag"
	).Link();

	Mesh* cubeMesh = mainScene->Resources()->Get<Mesh>("./res/models/not_cube.obj");
	Mesh* cubeMesh2 = mainScene->Resources()->Get<Mesh>("./res/models/not_cube2.obj");
	Mesh* schnozMesh = mainScene->Resources()->Get<Mesh>("./res/models/schnoz/schnoz.obj");
	Mesh* jakeMesh = mainScene->Resources()->Get<Mesh>("./res/models/jake_tangents.glb");

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

	playerNode->AddObject<Player>();/*
	MakeRooms(cubeMesh2, roomMat, mainScene, skyMat);
	AddEnemies(jakeMesh, schnozMat, mainScene, playerNode, cubeMesh2, reflectiveMat);*/

	

	JPH::Ref<JPH::CharacterVirtualSettings> characterSettings = new JPH::CharacterVirtualSettings();
	characterSettings->mShape = new JPH::CapsuleShape(1.0f, 0.5f);
	characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

	auto* virtualCharacter = playerNode->AddObject<Physics::VirtualCharacterController>(characterSettings);
	virtualCharacter->SetPosition(playerNode->GlobalTransform().Position().Value());
	virtualCharacter->Awake();

	auto mouseMarkerNode = mainScene->CreateNode("Mouse Marker");
	mouseMarkerNode->AddObject<MeshRenderer>(cubeMesh, reflectiveMat);
	mouseMarkerNode->GlobalTransform().Scale() = glm::vec3(0.15f, 0.02f, 0.15f);

	auto* bottleThrower = playerNode->AddObject<ThrowBottle>();
	bottleThrower->SetPoolSize(1);
	bottleThrower->SetResources(cubeMesh, reflectiveMat);

	auto* controller = playerNode->AddObject<PlayerController>(mouseMarkerNode);
	//controller->SetBottleThrower(bottleThrower);

	auto cameraNode = mainScene->CreateNode("Camera");
	Camera* camera = cameraNode->AddObject<Camera>(
		Camera::Perspective(25.0f, 16.0f / 9.0f, 0.1f, 200.0f));
	camera->SetAsMainCamera();
	cameraNode->AddObject<CameraSettings>(playerNode);

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

