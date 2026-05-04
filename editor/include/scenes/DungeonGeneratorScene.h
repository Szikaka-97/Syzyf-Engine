#pragma once

#include "GltfImporter.h"
#include "game_scripts/DungeonGenerator.h"

#include <AiNode.h>
#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
#include <Formatters.h>
#include <Framebuffer.h>
#include <Fxaa.h>
#include <InputSystem.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <MeshRenderer.h>
#include <ParticleSpawner.h>
#include <ReflectionProbe.h>
#include <ReflectionProbeSystem.h>
#include <Resources.h>
#include <Scene.h>
#include <Shader.h>
#include <Skybox.h>
#include <TimeSystem.h>
#include <Tonemapper.h>
#include <TweenSystem.h>
#include <Viewport.h>
#include <animation/AnimationSystem.h>
#include <fog/Fog.h>
#include <fog/FogVolume.h>
#include <fog/VolumetricFog.h>
#include <game_scripts/CameraSettings.h>
#include <game_scripts/PlayerController.h>
#include <game_scripts/ThrowBottle.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <physics/Body.h>
#include <physics/CharacterController.h>
#include <physics/ICollisionReceiver.h>
#include <physics/LayerMaskFilter.h>
#include <physics/System.h>
#include <physics/VirtualCharacterController.h>
#include <physics/Water.h>
#include <scatter/Spawner.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <imgui.h>

namespace DungeonGeneratorScene {

class Mover : public GameObject, public ImGuiDrawable {
  private:
    float pitch;
    float rotation;
    bool movementEnabled = false;
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
            float movementSpeed = this->movementSpeed;

            glm::vec3 right = this->GlobalTransform().Right();
            glm::vec3 up = glm::vec3(0, 1, 0);
            glm::vec3 forward = mode == 0 ? glm::cross(right, up)
                                          : this->GlobalTransform().Forward();

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
            if (GetScene()->Input()->KeyPressed(Key::E)) {
                movement += up;
            }
            if (GetScene()->Input()->KeyPressed(Key::Q)) {
                movement -= up;
            }
            if (GetScene()->Input()->KeyPressed(Key::LeftShift)) {
                movementSpeed *= 2;
            }

            if (glm::length(movement) > 0.0f) {
                movement = glm::normalize(movement);
            }

            this->GlobalTransform().Position() +=
                movement * (movementSpeed * Time::Delta());

            glm::vec2 deltaMovement = GetScene()->Input()->GetMouseMovement();

            this->rotation -= (deltaMovement.x / 20) * this->mouseSensitivity;
            this->pitch -= (deltaMovement.y / 20) * this->mouseSensitivity;

            if (this->rotation < -180) {
                this->rotation += 360;
            } else if (this->rotation > 180) {
                this->rotation -= 360;
            }

            this->pitch = glm::clamp(this->pitch, -89.0f, 89.0f);

            this->GlobalTransform().Rotation() =
                glm::angleAxis(glm::radians(this->rotation),
                               glm::vec3(0, 1, 0)) *
                glm::angleAxis(glm::radians(this->pitch), glm::vec3(1, 0, 0));

            this->GlobalTransform().Rotation() =
                this->GlobalTransform().Rotation().value;
        }

        if (GetScene()->Input()->KeyDown(Key::Escape)) {
            this->movementEnabled = !this->movementEnabled;
            GetScene()->Input()->SetMouseLocked(this->movementEnabled);

            if (this->movementEnabled) {
                glm::vec3 forward = this->GlobalTransform().Forward();
                this->pitch =
                    glm::degrees(asin(glm::clamp(-forward.y, -1.0f, 1.0f)));
                this->rotation = glm::degrees(atan2(forward.x, forward.z));
            }
        }
    }

    virtual void DrawImGui() {
        const char* modes[]{
            "Walking",
            "Freecam",
        };

        ImGui::Combo("Movement type", &this->mode, modes, 2);

        ImGui::InputFloat("Movement speed", &this->movementSpeed);
        ImGui::InputFloat("Mouse sensitivity", &this->mouseSensitivity);
    }
};

class PhysicsMover : public GameObject,
                     public Physics::ICollisionReceiver,
                     public ImGuiDrawable {
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

        this->character =
            this->GetObject<Physics::CharacterController>()->GetCharacter();
    }

    void Update() {
        if (cameraNode == nullptr) {
            this->cameraNode =
                this->GetNode()->GetObjectInChildren<Camera>()->GetNode();
            if (cameraNode == nullptr)
                return;
        }
        if (this->floorId.IsInvalid()) {
            this->floorId = this->GetScene()
                                ->FindObjectsOfType<Skybox>()
                                .front()
                                ->GetNode()
                                ->GetObject<Physics::Body>()
                                ->GetBodyID();
            if (this->floorId.IsInvalid())
                return;
        }

        JPH::Vec3 position = this->character->GetPosition();
        this->GlobalTransform().Position() = {position.GetX(), position.GetY(),
                                              position.GetZ()};

        if (movementEnabled) {
            glm::vec3 movement = glm::zero<glm::vec3>();
            glm::quat rotation = glm::identity<glm::quat>();

            glm::vec3 right = this->cameraNode->GlobalTransform().Right();
            glm::vec3 up = glm::vec3(0, 1, 0);
            glm::vec3 forward =
                mode == 0 ? glm::cross(right, up)
                          : this->cameraNode->GlobalTransform().Forward();
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
            } else if (this->rotation > 180) {
                this->rotation -= 360;
            }

            this->pitch = glm::clamp(this->pitch, -89.0f, 89.0f);
            this->cameraNode->LocalTransform().Rotation() =
                glm::angleAxis(glm::radians(this->rotation),
                               glm::vec3(0, 1, 0)) *
                glm::angleAxis(glm::radians(this->pitch), glm::vec3(1, 0, 0));

            JPH::Vec3 jphMovement = JPH::Vec3(movement.x, 0.0f, movement.z);

            JPH::Character::EGroundState groundState =
                this->character->GetGroundState();
            if (groundState == JPH::Character::EGroundState::OnSteepGround ||
                groundState == JPH::Character::EGroundState::NotSupported) {
                JPH::Vec3 normal = this->character->GetGroundNormal();
                normal.SetY(0.0f);
                float dot = normal.Dot(jphMovement);
                if (dot < 0.0f) {
                    jphMovement -= (dot * normal) / normal.LengthSq();
                }
            }

            if (this->character->IsSupported()) {
                JPH::Vec3 currentVelocity =
                    this->character->GetLinearVelocity();
                JPH::Vec3 desiredVelocity = this->movementSpeed * jphMovement;

                if (!desiredVelocity.IsNearZero() ||
                    currentVelocity.GetY() < 0.0f ||
                    !this->character->IsSupported()) {
                    desiredVelocity.SetY(currentVelocity.GetY());
                }
                JPH::Vec3 newVelocity =
                    0.75f * currentVelocity + 0.25f * desiredVelocity;

                if (jump &&
                    groundState == JPH::Character::EGroundState::OnGround) {
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
                this->cameraNode->GlobalTransform().Position().z};

            JPH::Vec3 direction =
                JPH::Vec3(this->cameraNode->GlobalTransform().Forward().x,
                          this->cameraNode->GlobalTransform().Forward().y,
                          this->cameraNode->GlobalTransform().Forward().z) *
                100.0f;

            Physics::LayerMaskFilter bodyFilter({1}, false);

            bodyFilter.IgnoreBody(this->character->GetBodyID());
            bodyFilter.IgnoreBody(this->floorId);

            SceneNode* result = physics->CastRay(
                this->cameraNode->GlobalTransform().Position(),
                this->cameraNode->GlobalTransform().Forward() * 100.0f, {}, {},
                bodyFilter);

            if (result) {
                if (auto* object = result->GetObject<Physics::Body>()) {
                    object->ApplyImpulse(
                        this->cameraNode->GlobalTransform().Forward() * 100.0f);
                    if (result->GetName() == "Physics Schnoz") {
                        heldItem = result;
                        object->OnDisable();
                    }
                }
            }
        }

        if (heldItem) {
            heldItem->GlobalTransform().Position() =
                this->cameraNode->GlobalTransform().Position() +
                this->cameraNode->GlobalTransform().Forward() * 2.0f;
        }

        if (GetScene()->Input()->ButtonDown(MouseButton::Right)) {
            auto* physics = this->GetScene()->GetComponent<Physics::System>();

            JPH::Vec3 direction =
                JPH::Vec3(this->cameraNode->GlobalTransform().Forward().x,
                          this->cameraNode->GlobalTransform().Forward().y,
                          this->cameraNode->GlobalTransform().Forward().z) *
                100.0f;

            JPH::ShapeRefC shape = new JPH::SphereShape(0.5f);

            std::vector<SceneNode*> results = physics->CastShape(
                this->cameraNode->GlobalTransform().Position(),
                this->cameraNode->GlobalTransform().Forward() * 100.0f, shape,
                {}, {},
                JPH::IgnoreSingleBodyFilter(this->character->GetBodyID()));
        }

        if (GetScene()->Input()->KeyDown(Key::Escape)) {
            this->movementEnabled = !this->movementEnabled;
            GetScene()->Input()->SetMouseLocked(this->movementEnabled);
        }
    }

    virtual void OnCollisionEnter(SceneNode* node) {}

    virtual void DrawImGui() {
        const char* modes[]{
            "Walking",
            "Freecam",
        };

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
    AutoRotator(float speed) { this->speed = speed; }

    void Update() {
        glm::quat rotation = glm::angleAxis(glm::radians(this->speed),
                                            glm::vec3(0.0f, 1.0f, 0.0f));
        this->LocalTransform().Rotation() *= rotation;
    }
};

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();

    ShaderProgram* skyProg = ShaderProgram::Build()
                                 .WithVertexShader("./res/shaders/skybox.vert")
                                 .WithPixelShader("./res/shaders/skybox.frag")
                                 .Link();

    ShaderProgram* coloredProg =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/lit.vert")
            .WithPixelShader("./res/shaders/lambert color.frag")
            .Link();

    ShaderProgram* diffuseTexProg =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/lit.vert")
            .WithPixelShader("./res/shaders/lambert.frag")
            .Link();

    ShaderProgram* pbrProg = ShaderProgram::Build()
                                 .WithVertexShader("./res/shaders/lit.vert")
                                 .WithPixelShader("./res/shaders/pbr.frag")
                                 .Link();

    ShaderProgram* pbrRefractProg =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/lit.vert")
            .WithPixelShader("./res/shaders/pbr refract.frag")
            .Link();

    ShaderProgram* transparentProg =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/lit.vert")
            .WithPixelShader("./res/shaders/transparent.frag")
            .Link();

    Mesh* cannonMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/cannon/cannon.obj");
    // Mesh* cubeMesh =
    //     mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");
    Mesh* tvMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/tv_stand.fbx");
    Mesh* schnozMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/schnoz/schnoz.obj");

    Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
        "./res/textures/citrus_orchard_road_puresky.hdr",
        Texture::HDRColorBuffer);
    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

    Texture2D* cannonDiffuse = mainScene.Resources()->Get<Texture2D>(
        "./res/models/cannon/textures/cannon_01_diff_1k.png",
        Texture::ColorTextureRGB);
    Texture2D* cannonNormal = mainScene.Resources()->Get<Texture2D>(
        "./res/models/cannon/textures/cannon_01_nor_gl_1k.png",
        Texture::TechnicalMapXYZ);
    Texture2D* cannonARM = mainScene.Resources()->Get<Texture2D>(
        "./res/models/cannon/textures/cannon_01_arm_1k.png",
        Texture::TechnicalMapXYZ);

    Texture2D* reflectiveDiffuse = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-shiny-metal-albedo.png",
        Texture::ColorTextureRGB);
    Texture2D* reflectiveNormal = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-shiny-metal-Normal-ogl.png",
        Texture::TechnicalMapXYZ);
    Texture2D* reflectiveARM = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-shiny-metal-arm.png",
        Texture::TechnicalMapXYZ);
    Texture2D* roughARM = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-rough-metal-arm.png",
        Texture::TechnicalMapXYZ);
    Texture2D* shinyNonMetalARM = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-shiny-nonmetal-arm.png",
        Texture::TechnicalMapXYZ);

    Texture2D* schnozTexture = mainScene.Resources()->Get<Texture2D>(
        "./res/models/schnoz/Diffuse.png", Texture::ColorTextureRGB);

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
    screenMat->SetValue(
        "colorTex",
        (Texture2D*)schnozPreview->GetFramebuffer()->GetColorTexture());

    Material* schnozMat = new Material(diffuseTexProg);
    schnozMat->SetValue("uColor", glm::vec3(1, 1, 1));
    schnozMat->SetValue("colorTex", schnozTexture);

    Material* blueTransparentMat = new Material(transparentProg);
    blueTransparentMat->SetValue("uColor", glm::vec4(0.5, 0.5, 1.0, 0.6));

    // SceneNode* playerNode = mainScene.CreateNode("Player");
    // playerNode->GlobalTransform().Position() = glm::vec3(0.0f, 5.0f, -10.0f);
    // JPH::Ref<JPH::CharacterSettings> characterSettings =
    //     new JPH::CharacterSettings();
    // characterSettings->mShape = new JPH::CapsuleShape(1.0f, 0.5f);
    // characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
    // characterSettings->mFriction = 0.5f;
    // characterSettings->mLayer = Physics::Layers::MOVING;
    // playerNode->AddObject<Physics::CharacterController>(characterSettings);
    // playerNode->AddObject<PhysicsMover>();

    auto cameraNode = mainScene.CreateNode("Camera");
    Camera* camera = cameraNode->AddObject<Camera>(
        Camera::Perspective(40.0f, 16.0f / 9.0f, 0.5f, 200.0f));
    // camera->GlobalTransform().Position() = glm::vec3(0.0f, 5.0f, -10.0f);
    //
    auto floorNode = mainScene.CreateNode("Floor");
    floorNode->GlobalTransform().Position() -= glm::vec3(0.0f, 0.5f, 0.0f);
    floorNode->AddObject<Skybox>(skyMat);
    floorNode->AddObject<Physics::Body>(JPH::BodyCreationSettings(
        Physics::BoxShape({50.0f, 0.5f, 50.0f}), JPH::RVec3::sZero(),
        JPH::Quat::sIdentity(), JPH::EMotionType::Static,
        Physics::Layers::NON_MOVING));

    auto lightNode = mainScene.CreateNode("Point Light");
    lightNode->AddObject<Light>(Light::PointLight({1, 1, 1}, 10, 2))
        ->SetShadowCasting(false);
    lightNode->GlobalTransform().Position() = {-1, 2.2f, 0};

    auto lightNode2 = mainScene.CreateNode("Directional Light");
    lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 1))
        ->SetShadowCasting(true);
    lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
    lightNode2->GlobalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(64.0f, 0.0f, 0.0f)));

    // SceneNode* schnozCameraNode = mainScene.CreateNode("Schnoz Camera");
    // schnozCameraNode->LocalTransform().Position() = glm::vec3(-56.5, 2.0,
    // -2.0); schnozCameraNode->LocalTransform().Rotation() =
    //     glm::quat(glm::radians(glm::vec3(5.0f, 85.0f, 0.0f)));
    //
    // auto schnozCamera = schnozCameraNode->AddObject<Camera>(
    //     Camera::Perspective(40.0f, 16.0f / 9.0f, 0.5f, 200.0f));
    // schnozCamera->SetAspectRatio(2);
    // schnozCamera->SetRenderTarget(schnozPreview);
    // schnozCamera->SetLayerMask(uint8_t(5));

    // SceneNode* schnozNode = mainScene.CreateNode("Schnoz");
    // schnozNode->LocalTransform().Position() = glm::vec3(-53.5, 1.75, -2.4);
    // schnozNode->LocalTransform().Scale() = glm::vec3(0.15, 0.15, 0.15);
    // schnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);
    // schnozNode->AddObject<AutoRotator>(1);
    // schnozNode->SetLayer(5);
    //
    // SceneNode* w_schnozNode = mainScene.CreateNode("w_schnozNode");
    // w_schnozNode->LocalTransform().Position() = glm::vec3(-20, 0, -20);
    // schnozNode->LocalTransform().Scale() = glm::vec3(1, 1, 1);
    // w_schnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);
    //
    // JPH::ShapeRefC w_schnozShape = Physics::ConvexHullMeshShape(schnozMesh);
    // JPH::BodyCreationSettings w_schnozShapeSettings = {
    //     w_schnozShape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(),
    //     JPH::EMotionType::Dynamic, Physics::Layers::MOVING};

    // auto* w_schnozBody =
    //     w_schnozNode->AddObject<Physics::Body>(w_schnozShapeSettings);
    // w_schnozBody->SetRestitution(0.0f);
    // w_schnozBody->SetFriction(0.5f);
    // w_schnozBody->SetLinearDamping(0.1f);
    // w_schnozBody->SetCollisionLayerAndMask({0});
    //
    // SceneNode* schnozLightNode = mainScene.CreateNode("Schnoz Light");
    // schnozLightNode->LocalTransform().Position() = glm::vec3(-55.5, 3.0,
    // -2.0); schnozLightNode->AddObject<Light>(
    //     Light::PointLight(glm::vec3(1, 1, 1), 5, 5));

    // JPH::ShapeRefC schnozShape = Physics::ConvexHullMeshShape(schnozMesh);
    // SceneNode* schnozRootNode = mainScene.CreateNode("Schnoz Root");
    // for (int i = 0; i < 50; ++i) {
    //     SceneNode* physicsSchnozNode =
    //         mainScene.CreateNode(schnozRootNode, "Physics Schnoz");
    //     physicsSchnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);
    //     physicsSchnozNode->GlobalTransform().Position() = {
    //         2.0f + i, 10.0f + i * 2.0f, 0.0f - i};
    //     physicsSchnozNode->GlobalTransform().Scale() = glm::vec3(0.25f);
    //     JPH::BodyCreationSettings schnozShapeSettings = {
    //         schnozShape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(),
    //         JPH::EMotionType::Dynamic, Physics::Layers::MOVING};
    //     auto* schnozBody =
    //         physicsSchnozNode->AddObject<Physics::Body>(schnozShapeSettings);
    //
    //     schnozBody->SetCollisionLayerAndMask({0});
    // }

    // cameraNode->AddObject<Bloom>();
    // cameraNode->AddObject<Tonemapper>()->SetOperator(
    //     Tonemapper::TonemapperOperator::GranTurismo);

    SceneNode* dungeon = mainScene.CreateNode("Dungeon");
    dungeon->AddObject<DungeonGenerator>(
        DungeonGeneratorSettings{.steps = 12,
                                 .mapColumns = 4,
                                 .mapRows = 10,
                                 .momentum = 2.0f,
                                 .horizontalBias = 0.0f});

    // mainScene.AddComponent<DebugInspector>();
    // mainScene.AddComponent<AnimationSystem>();

    // // ---- PLAYER ----
    // SceneNode* playerNode = mainScene.CreateNode("Player");
    // SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    // playerNode->GlobalTransform().Position() = glm::vec3(0.0f, 10.0f, 0.0f);
    // cameraNode->AddObject<Camera>(
    //     Camera::Perspective(25.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    // cameraNode->AddObject<CameraSettings>(playerNode);
    auto* fog = cameraNode->AddObject<Fog>();
    fog->fogType = Fog::Type::Atmospheric;
    fog->density = 0.0042;
    fog->fogColor = {0.2, 0.6, 0.9, 1.0};
    auto* fog2 = cameraNode->AddObject<Fog>();
    cameraNode->AddObject<Bloom>();
    cameraNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    cameraNode->AddObject<ColorGrading>();
    cameraNode->AddObject<Fxaa>();

    // JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
    //     new JPH::CharacterVirtualSettings();
    // characterSettings->mShape = new JPH::CapsuleShape(0.2f, 0.7f);
    // characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

    // auto* virtualCharacter =
    //     playerNode->AddObject<Physics::VirtualCharacterController>(
    //         characterSettings);
    // virtualCharacter->SetPosition(
    //     playerNode->GlobalTransform().Position().Value());

    // auto mouseMarkerNode = mainScene.CreateNode("Mouse Marker");
    // mouseMarkerNode->GlobalTransform().Scale() = glm::vec3(0.15f, 0.02f,
    // 0.15f);

    // auto* bottleThrower = playerNode->AddObject<ThrowBottle>();
    // bottleThrower->SetPoolSize(10);
    // auto* controller =
    // playerNode->AddObject<PlayerController>(mouseMarkerNode);
    // controller->SetBottleThrower(bottleThrower);

    // SceneNode* playerMeshNode = mainScene.CreateNode(playerNode);
    // playerMeshNode->AddObject<MeshRenderer>(schnozMesh, reflectiveMat);
    // playerMeshNode->GlobalTransform().Position() = glm::vec3(0.0f, 5.0f,
    // 0.0f); playerMeshNode->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f,
    // 0.5f); Mesh* cubeMesh =
    //     mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");
    // bottleThrower->SetResources(cubeMesh, reflectiveMat);
}
} // namespace DungeonGeneratorScene
