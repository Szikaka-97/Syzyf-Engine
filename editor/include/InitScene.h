#pragma once

#include <AiNode.h>
#include <Bloom.h>
#include <Camera.h>
#include <Framebuffer.h>
#include <InputSystem.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <MeshRenderer.h>
#include <ParticleSpawner.h>
#include <ReflectionProbe.h>
#include <Scene.h>
#include <Shader.h>
#include <Skybox.h>
#include <TimeSystem.h>
#include <Tonemapper.h>
#include <Viewport.h>
#include <animation/AnimationSystem.h>
#include <fog/FogVolume.h>
#include <physics/Body.h>
#include <physics/DebugRenderer.h>
#include <physics/System.h>
#include <physics/Water.h>

#include <imgui.h>

class EditorCameraTag : public GameObject {};

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
    glm::vec2 uvSpeed = {2.0f, 1.0f};

  public:
    Tornado() {
        this->tornadoMesh = this->GetScene()->Resources()->Get<Mesh>(
            "./res/models/tornado/tornado.obj");
        this->tornadoShader.reset(
            ShaderProgram::Build()
                .WithVertexShader(GetScene()->Resources()->Get<VertexShader>(
                    "./res/shaders/lit.vert"))
                .WithPixelShader(GetScene()->Resources()->Get<PixelShader>(
                    "./res/shaders/tornado.frag"))
                .Link());
        this->tornadoShader->SetCastsShadows(false);
        this->tornadoShader->SetTransparent(true);
        this->tornadoMaterial =
            std::make_unique<Material>(this->tornadoShader.get());
        this->tornadoMaterial->SetValue("uOpacity", this->opacity);
        this->tornadoMaterial->SetValue("uSpeed", this->uvSpeed);
        this->tornadoMaterial->SetValue("uIntensity", this->intensity);

        this->tornadoMaterial->SetValue(
            "uNoiseTexture", this->GetScene()->Resources()->Get<Texture2D>(
                                 "./res/textures/noise/T_Noise_HU85k.png",
                                 Texture::TechnicalMapXYZ));
        this->tornadoMaterial->SetValue(
            "uColorGradientTexture",
            this->GetScene()->Resources()->Get<Texture2D>(
                "./res/textures/inkpink-alpha.png", Texture::ColorTextureRGBA));
        this->tornadoMaterial->SetValue(
            "uSubtractionTexture",
            this->GetScene()->Resources()->Get<Texture2D>(
                "./res/textures/subtraction.png", Texture::TechnicalMapXYZ));

        this->tornadoNode =
            this->GetScene()->CreateNode(this->GetNode(), "Tornado");
        this->tornadoNode->AddObject<MeshRenderer>(this->tornadoMesh,
                                                   this->tornadoMaterial.get());

        this->particlesBottom = this->GetScene()->CreateNode(
            this->GetNode(), "Tornado Particles Bottom");
        this->particlesMesh = this->GetScene()->Resources()->Get<Mesh>(
            "./res/models/tornado/tornado_base.obj");
        this->particlesShader.reset(
            ShaderProgram::Build()
                .WithVertexShader(
                    this->GetScene()->Resources()->Get<VertexShader>(
                        "./res/shaders/particles/particles.vert"))
                .WithPixelShader(
                    this->GetScene()->Resources()->Get<PixelShader>(
                        "./res/shaders/particles/tornado_base.frag"))
                .Link());
        this->particlesShader->SetCastsShadows(false);
        this->particlesShader->SetIgnoresDepthPrepass(true);
        this->particlesShader->SetTransparent(true);

        this->particlesBottomMaterial =
            std::make_unique<Material>(this->particlesShader.get());
        this->particlesTopMaterial =
            std::make_unique<Material>(this->particlesShader.get());

        this->particlesBottomMaterial->SetValue(
            "colorTex", this->GetScene()->Resources()->Get<Texture2D>(
                            "./res/textures/noise/T_FirePanningCyl45.png",
                            Texture::TechnicalMapXYZ));

        Texture2D* colorRampTex = this->GetScene()->Resources()->Get<Texture2D>(
            "./res/textures/inkpink-32x.png", Texture::ColorTextureRGB);
        this->particlesBottomMaterial->SetValue("colorRamp", colorRampTex);

        ParticleSpawnerSettings particleSettings = {
            .maxParticles = 8,
            .emissionShapeExtents = glm::vec3(0.01f),

            .minVelocity = {0.0f, 0.0f, 0.0f},
            .maxVelocity = {0.0f, 0.2f, 0.0f},

            .minInitialAngle = glm::radians(-180.0f),
            .maxInitialAngle = glm::radians(180.0f),
            .minAngularVelocity = -4.0f,
            .maxAngularVelocity = 8.0f,
            .rotateY = true,

            .enableLifetime = true,
            .minLifetime = 2.0f,
            .maxLifetime = 3.0f,

            .minScale = 1.5f,
            .maxScale = 1.8f,

            .lifetimeFadeMode = FadeMode::Alpha,
            .lifetimeFadeIn = {0.0f, 0.1f},
            .lifetimeFadeOut = {0.9f, 1.0f},

            .wrapAround = false,
            .continuous = true,
            .useColorRamp = true,
        };

        this->particlesBottom->AddObject<ParticleSpawner>(
            this->particlesMesh, std::move(particlesBottomMaterial),
            particleSettings);

        this->particleTop = this->GetScene()->CreateNode(
            this->GetNode(), "Tornado Particles Top");
        this->particlesTopMaterial->SetValue(
            "colorTex", this->GetScene()->Resources()->Get<Texture2D>(
                            "./res/textures/noise/T_FirePanningCyl45.png",
                            Texture::TechnicalMapXYZ));
        this->particlesTopMaterial->SetValue("colorRamp", colorRampTex);

        particleSettings.minScale = 3.5f;
        particleSettings.maxScale = 4.0f;
        this->particleTop->AddObject<ParticleSpawner>(
            this->particlesMesh, std::move(particlesTopMaterial),
            particleSettings);
        this->particleTop->LocalTransform().Position() = {0.0f, 4.8f, 0.0f};
    }
    //
    // void Update() {
    //     glm::quat rotation =
    //     glm::angleAxis(glm::radians(this->rotationSpeed),
    //                                         glm::vec3(0.0f, 1.0f, 0.0f));
    //     glm::quat newRotation =
    //         this->tornadoNode->LocalTransform().Rotation() * rotation;
    //
    //     this->tornadoNode->LocalTransform().Rotation() =
    //         glm::normalize(newRotation);
    // }

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

inline void InitScene(Scene& mainScene, Camera*& mainCamera) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<Physics::DebugRenderer>();

    ShaderProgram* skyProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/skybox.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/skybox.frag"))
            .Link();

    ShaderProgram* coloredProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/lambert color.frag"))
            .Link();

    ShaderProgram* diffuseTexProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/lambert.frag"))
            .Link();

    ShaderProgram* pbrProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/pbr.frag"))
            .Link();

    ShaderProgram* pbrRefractProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/pbr refract.frag"))
            .Link();

    ShaderProgram* transparentProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/transparent.frag"))
            .Link();
    transparentProg->SetTransparent(true);

    Mesh* gmConstructMesh = mainScene.Resources()->Get<Mesh>(
        "./res/models/construct/construct.obj", true);
    Mesh* cannonMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/cannon/cannon.obj");
    Mesh* cubeMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");
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

    // auto constructNode = mainScene->CreateNode("gm_construct");
    // constructNode->AddObject<MeshRenderer>(gmConstructMesh,
    // gmConstructMesh->GetDefaultMaterials());
    //  constructNode->AddObject<Physics::Body>(Physics::Body::Mesh(gmConstructMesh,
    //  JPH::EMotionType::Static, Physics::Layers::NON_MOVING));

    auto cannonNode = mainScene.CreateNode("Cannon");
    cannonNode->AddObject<MeshRenderer>(cannonMesh, cannonMat);

    auto cubeNode = mainScene.CreateNode("Reflective Cube");
    cubeNode->AddObject<MeshRenderer>(cubeMesh, reflectiveMat);
    cubeNode->GlobalTransform().Position() = {-2.0f, 1.0f, 0.0f};
    cubeNode->GlobalTransform().Scale() = glm::vec3(0.6f);

    auto roughCubeNode = mainScene.CreateNode(cubeNode, "Rough Cube");
    roughCubeNode->AddObject<MeshRenderer>(cubeMesh, roughMat);
    roughCubeNode->LocalTransform().Position() = {0, 0, 3};

    auto shinyCubeNode = mainScene.CreateNode(cubeNode, "Shiny Cube");
    shinyCubeNode->AddObject<MeshRenderer>(cubeMesh, shinyMat);
    shinyCubeNode->LocalTransform().Position() = {0, 0, -3};

    auto cubeNode2 = mainScene.CreateNode("Reflective Cube");
    cubeNode2->AddObject<MeshRenderer>(cubeMesh, reflectiveMat);
    cubeNode2->GlobalTransform().Position() = {-25.0f, 1.0f, 0.0f};
    cubeNode2->GlobalTransform().Scale() = glm::vec3(0.6f);

    auto roughCubeNode2 = mainScene.CreateNode(cubeNode2, "Rough Cube");
    roughCubeNode2->AddObject<MeshRenderer>(cubeMesh, roughMat);
    roughCubeNode2->LocalTransform().Position() = {0, 0, 3};

    auto shinyCubeNode2 = mainScene.CreateNode(cubeNode2, "Shiny Cube");
    shinyCubeNode2->AddObject<MeshRenderer>(cubeMesh, shinyMat);
    shinyCubeNode2->LocalTransform().Position() = {0, 0, -3};

    SceneNode* playerNode = mainScene.CreateNode("Player");
    playerNode->GlobalTransform().Position() = glm::vec3(2.0f, 2.0f, -10.0f);
    playerNode->AddObject<Mover>();

    // MOVE OUTSIDE OF HERE
    mainCamera = playerNode->AddObject<Camera>(
        Camera::Perspective(40.0f, 16.0f / 9.0f, 0.5f, 200.0f));
    mainCamera->GlobalTransform().Position() = glm::vec3(2.0f, 2.0f, -10.0f);
    mainCamera->AddObject<EditorCameraTag>();

    // auto* floorMesh =
    // mainScene->Resources()->Get<Mesh>("./res/models/floor/floor.obj", true);
    auto floorNode = mainScene.CreateNode("Floor");
    floorNode->AddObject<MeshRenderer>(gmConstructMesh,
                                       gmConstructMesh->GetDefaultMaterials());
    floorNode->AddObject<Skybox>(skyMat);
    floorNode->AddObject<Physics::Body>(
        Physics::Body::Mesh(gmConstructMesh, JPH::EMotionType::Static,
                            Physics::Layers::NON_MOVING));
    floorNode->AddObject<Surface>(gmConstructMesh, 1.0f);

    auto lightNode = mainScene.CreateNode("Point Light");
    lightNode->AddObject<Light>(Light::PointLight({1, 1, 1}, 10, 2))
        ->SetShadowCasting(false);
    lightNode->GlobalTransform().Position() = {-1, 2.2f, 0};

    auto lightNode2 = mainScene.CreateNode("Directional Light");
    lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 4))
        ->SetShadowCasting(true);
    lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
    lightNode2->GlobalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(64.0f, 0.0f, 0.0f)));

    auto envProbe = mainScene.CreateNode(cubeNode, "Reflection Probe");
    envProbe->AddObject<ReflectionProbe>();

    auto envProbe2 = mainScene.CreateNode("Reflection Probe");
    envProbe2->AddObject<ReflectionProbe>();
    envProbe2->GlobalTransform().Position() = {-10.0f, 1.5f, 0.6f};

    auto envProbe3 = mainScene.CreateNode("Reflection Probe");
    envProbe3->AddObject<ReflectionProbe>();
    envProbe3->GlobalTransform().Position() = {-29.0f, 1.5f, 0.6f};

    auto envProbe4 = mainScene.CreateNode(shinyCubeNode, "Reflection Probe");
    envProbe4->AddObject<ReflectionProbe>();

    // auto starsAttachmentNode = mainScene->CreateNode("Stars Scene
    // Attachment");
    //
    // auto starsScene = new Scene();
    //
    // auto starsNode = starsScene->CreateNode("Stars");
    // starsNode->AddObject<Stars>(1000);
    // starsNode->GlobalTransform().Position() = {-15.0f, 5.5f, -105.0f};
    //
    // starsAttachmentNode->AttachScene(starsScene);

    SceneNode* tvNode = mainScene.CreateNode("TV");
    tvNode->LocalTransform().Scale() = glm::vec3(1.5, 1.5, 1.5);
    tvNode->LocalTransform().Position() = glm::vec3(3, -5, -2);
    tvNode->LocalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(-90.0f, 20.0f, 0.0f)));

    auto tvRenderer = tvNode->AddObject<MeshRenderer>(tvMesh, nullptr);
    tvRenderer->SetMaterial(tvMatStand, 0);
    tvRenderer->SetMaterial(screenMat, 1);
    tvRenderer->SetMaterial(tvMatStand, 2);
    tvRenderer->SetMaterial(tvMatStand, 3);

    SceneNode* fogVolume = mainScene.CreateNode("Fog Volume");
    FogVolume* fogVolumeObject = fogVolume->AddObject<FogVolume>();
    fogVolumeObject->stepSize = 0.06f;
    fogVolumeObject->scatteringDensity = 0.042f;
    fogVolumeObject->absorptionDensity = 0.0f;
    fogVolumeObject->k = 0.005f;
    fogVolume->GlobalTransform().Position() = {-28.0f, 1.5f, 0.0f};
    fogVolume->GlobalTransform().Scale() = {20.0f, 12.0f, 20.0f};

    SceneNode* fogVolume2 = mainScene.CreateNode("Fog Volume 2");
    FogVolume* fogVolume2Object = fogVolume2->AddObject<FogVolume>();
    fogVolume2Object->scatteringColor = {173, 0, 255};
    fogVolume2Object->stepSize = 0.03f;
    fogVolume2Object->scatteringDensity = 2.0f;
    fogVolume2Object->absorptionDensity = 0.0f;
    fogVolume2->GlobalTransform().Position() = {0.0f, 0.6f, 3.0f};

    SceneNode* schnozCameraNode = mainScene.CreateNode("Schnoz Camera");
    schnozCameraNode->LocalTransform().Position() = glm::vec3(-56.5, 2.0, -2.0);
    schnozCameraNode->LocalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(5.0f, 85.0f, 0.0f)));

    auto schnozCamera = schnozCameraNode->AddObject<Camera>(
        Camera::Perspective(40.0f, 16.0f / 9.0f, 0.5f, 200.0f));
    schnozCamera->SetAspectRatio(2);
    schnozCamera->SetRenderTarget(schnozPreview);
    schnozCamera->SetLayerMask(uint8_t(5));

    SceneNode* schnozNode = mainScene.CreateNode("Schnoz");
    schnozNode->LocalTransform().Position() = glm::vec3(-53.5, 1.75, -2.4);
    schnozNode->LocalTransform().Scale() = glm::vec3(0.15, 0.15, 0.15);
    schnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);
    schnozNode->SetLayer(5);

    SceneNode* w_schnozNode = mainScene.CreateNode("w_schnozNode");
    w_schnozNode->LocalTransform().Position() = glm::vec3(-20, 0, -20);
    schnozNode->LocalTransform().Scale() = glm::vec3(1, 1, 1);
    w_schnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);

    JPH::BodyCreationSettings w_schnozShapeSettings =
        Physics::Body::ConvexHullMesh(schnozMesh, JPH::EMotionType::Dynamic,
                                      Physics::Layers::MOVING);
    auto* w_schnozBody =
        w_schnozNode->AddObject<Physics::Body>(w_schnozShapeSettings);
    w_schnozBody->SetRestitution(0.0f);
    w_schnozBody->SetFriction(0.5f);
    w_schnozBody->SetLinearDamping(0.1f);
    w_schnozBody->Awake();
    w_schnozBody->SetCollisionLayerAndMask({0});

    auto enemyAI = w_schnozNode->AddObject<AiNode>();
    if (enemyAI) {
        enemyAI->SetTarget(playerNode);
    }

    glm::vec2 patrolPoints[] = {glm::vec2(-20, 0), glm::vec2(-40, 0)};

    /*auto aiNode = w_schnozNode->GetObject<AiNode>();
    if (aiNode) {
            aiNode->SetPatrolPoints(patrolPointsVec);
    }*/

    std::vector<glm::vec2> patrolPointsVec(std::begin(patrolPoints),
                                           std::end(patrolPoints));
    w_schnozNode->GetObject<AiNode>()->SetPatrolPoints(patrolPointsVec);

    SceneNode* schnozLightNode = mainScene.CreateNode("Schnoz Light");
    schnozLightNode->LocalTransform().Position() = glm::vec3(-55.5, 3.0, -2.0);
    schnozLightNode->AddObject<Light>(
        Light::PointLight(glm::vec3(1, 1, 1), 5, 5));

    // cameraNode->AddObject<VolumetricFog>();
    for (int i = 0; i < 50; ++i) {
        SceneNode* physicsSchnozNode = mainScene.CreateNode("Physics Schnoz");
        physicsSchnozNode->AddObject<MeshRenderer>(schnozMesh, schnozMat);
        physicsSchnozNode->GlobalTransform().Position() = {
            2.0f + i, 10.0f + i * 2.0f, 0.0f - i};
        physicsSchnozNode->GlobalTransform().Scale() = glm::vec3(0.25f);
        JPH::BodyCreationSettings schnozShapeSettings =
            Physics::Body::ConvexHullMesh(schnozMesh, JPH::EMotionType::Dynamic,
                                          Physics::Layers::MOVING);
        auto* schnozBody =
            physicsSchnozNode->AddObject<Physics::Body>(schnozShapeSettings);

        schnozBody->SetCollisionLayerAndMask({0});
    }

    playerNode->AddObject<Bloom>();
    playerNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    // playerNode->AddObject<Fog>();

    Mesh* waterMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/water.obj");
    SceneNode* water = mainScene.CreateNode("Water");
    water->AddObject<MeshRenderer>(waterMesh, blueTransparentMat);
    water->AddObject<Physics::Water>();
    water->GlobalTransform().Position() = {4.0f, 0.0f, -8.0f};
    auto* waterBody =
        water->AddObject<Physics::Body>(Physics::Body::ConvexHullMesh(
            waterMesh, JPH::EMotionType::Static, Physics::Layers::NON_MOVING));
    waterBody->SetIsSensor(true);
    waterBody->SetCollisionLayerAndMask({1});

    // SceneNode* jakeAttachment = mainScene.CreateNode("Jake");
    // Scene* animatedGltfScene =
    //     GltfImporter::LoadScene("./res/models/jake_tangents.glb", "Jake");
    // jakeAttachment->AttachScene(animatedGltfScene);

    SceneNode* skeletonAttachment = mainScene.CreateNode("Skeleton");
    Scene* skeletonGltfScene =
        GltfImporter::LoadScene("./res/models/szkielet5.glb", "Szkielet");
    skeletonAttachment->AttachScene(skeletonGltfScene);

    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<AnimationSystem>();

    SceneNode* tornadoNode = mainScene.CreateNode("Tornado Node");
    tornadoNode->AddObject<Tornado>();
    tornadoNode->GlobalTransform().Position() = {5.0f, 0.0f, 0.0f};
}
