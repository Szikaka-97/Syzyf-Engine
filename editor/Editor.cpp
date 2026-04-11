#include "include/Editor.h"

#include "thirdparty/ImGuizmo.h"

#include <algorithm>
#include <imgui.h>
#define IMVIEWGUIZMO_IMPLEMENTATION
#include "thirdparty/ImViewGuizmo.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <filesystem>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl/imgui_impl_opengl3.h>
#include <imgui_impl/imgui_impl_sdl3.h>
#include <spdlog/spdlog.h>

#include <AiNode.h>
#include <Bloom.h>
#include <Engine.h>
#include <Framebuffer.h>
#include <Graphics.h>
#include <InputSystem.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/RegisterTypes.h>
#include <Light.h>
#include <MeshRenderer.h>
#include <ReflectionProbe.h>
#include <Scene.h>
#include <Skybox.h>
#include <Texture.h>
#include <TimeSystem.h>
#include <Tonemapper.h>
#include <Viewport.h>
#include <animation/AnimationSystem.h>
#include <fog/Fog.h>
#include <fog/FogVolume.h>
#include <physics/Body.h>
#include <physics/CharacterController.h>
#include <physics/DebugRenderer.h>
#include <physics/Jolt.h>
#include <physics/System.h>
#include <physics/Water.h>

namespace Editor {
const char* GLSL_VERSION = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;

// Move into some struct
SceneNode* selectedNode = nullptr;
Camera* mainCamera = nullptr;
ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

bool InitProgram() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("Failed to initialize SDL3: {}", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_VERSION_MINOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("Syzyf Editor", 1280, 720,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        spdlog::error("Failed to initialize GLAD");
        return false;
    }

    Engine::window = window;

    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    JPH::Trace = Physics::TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = Physics::AssertFailedImpl;
#endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    return true;
}

bool InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.IniFilename = "imgui.ini";

    if (!std::filesystem::exists("imgui.ini")) {
        if (std::filesystem::exists("default_editor_layout.ini")) {
            ImGui::LoadIniSettingsFromDisk("default_editor_layout.ini");
        }
    }

    // Add a toggle, add a custom
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    return true;
}

bool Setup() { return InitProgram() && InitImGui(); }

void Terminate() {
    ImGui::DestroyPlatformWindows();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

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

void InitScene(Scene& mainScene) {
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

    Scene* animatedGltfScene = GltfImporter::LoadScene(
        "./res/models/jake_tangents.glb", "Animated Gltf");
    mainScene.GetRootNode()->AttachScene(animatedGltfScene);

    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<AnimationSystem>();
}

void DrawMainMenuBar(bool& shouldClose) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                shouldClose = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark")) {
                    ImGui::StyleColorsDark();
                }
                if (ImGui::MenuItem("Light")) {
                    ImGui::StyleColorsLight();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void DrawGraphNode(SceneNode& node) {
    ImGui::PushID(node.GetID());

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    std::string treeHeader = node.GetName();
    if (treeHeader.empty()) {
        treeHeader = std::to_string(node.GetID());
    }

    bool isLeaf = node.GetChildren().empty();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (&node == node.GetScene()->GetRootNode()) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (selectedNode == &node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (isLeaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)node.GetID(), flags,
                                      "%s", treeHeader.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedNode = &node;
    }

    ImGui::TableNextColumn();

    const bool isEnabled = node.IsEnabled();

    if (!isEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    if (ImGui::Button("E", ImVec2(24, ImGui::GetFrameHeight()))) {
        node.SetEnabled(!node.IsEnabled());
    }

    if (!isEnabled)
        ImGui::PopStyleColor(3);

    if (nodeOpen) {
        if (!isLeaf) {
            for (SceneNode* child : node.GetChildren()) {
                DrawGraphNode(*child);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DrawGraph(Scene& scene) {
    ImGui::Begin("Graph");

    SceneNode* root = scene.GetRootNode();

    if (root != nullptr) {
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f);

        ImGuiTableFlags tableFlags =
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Graph Table", 2, tableFlags)) {
            ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthStretch);

            // change so the 30px isnt hardcoded
            ImGui::TableSetupColumn("Visibility",
                                    ImGuiTableColumnFlags_WidthFixed, 30.0f);

            DrawGraphNode(*root);

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void DrawInspector() {
    ImGui::Begin("Inspector");
    if (selectedNode != nullptr) {
        std::string name = selectedNode->GetName();
        if (name.empty()) {
            ImGui::TextUnformatted(
                std::to_string(selectedNode->GetID()).c_str());
        } else {
            ImGui::TextUnformatted(name.c_str());
        }

        bool nodeEnabled = selectedNode->IsEnabled();
        ImGui::Checkbox("Enabled", &nodeEnabled);
        selectedNode->SetEnabled(nodeEnabled);

        if (ImGui::TreeNode("Layer")) {
            const float size = ImGui::CalcTextSize("00").x;

            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 8; x++) {
                    if (x > 0) {
                        ImGui::SameLine();
                    }

                    uint8_t layer = y * 8 + x;

                    ImGui::PushID(layer);

                    if (ImGui::Selectable(std::to_string(layer).c_str(),
                                          selectedNode->GetLayer() == layer, 0,
                                          ImVec2(size, size))) {
                        selectedNode->SetLayer(layer);
                    }

                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Transform")) {
            ImGui::Text("Position");

            glm::vec3 position = selectedNode->GlobalTransform().Position();

            ImGui::InputFloat3("##Position", &position[0]);

            glm::vec3 positionDelta = glm::zero<glm::vec3>();

            ImGui::SliderFloat3("##PositionDelta", &positionDelta[0], -1, 1);

            position += positionDelta;

            selectedNode->GlobalTransform().Position() = position;

            ImGui::Text("Rotation");

            glm::vec3 rotationEuler = glm::degrees(glm::eulerAngles(
                selectedNode->GlobalTransform().Rotation().Value()));

            ImGui::InputFloat3("##Rotation", &rotationEuler[0]);

            selectedNode->GlobalTransform().Rotation() =
                glm::quat(glm::radians(rotationEuler));

            glm::vec3 rotationDelta = glm::zero<glm::vec3>();

            ImGui::SliderFloat3("##RotationDelta", &rotationDelta[0], -1, 1);

            selectedNode->GlobalTransform().Rotation() *=
                glm::angleAxis(glm::radians(rotationDelta.x),
                               glm::vec3(1, 0, 0)) *
                glm::angleAxis(glm::radians(rotationDelta.y),
                               glm::vec3(0, 1, 0)) *
                glm::angleAxis(glm::radians(rotationDelta.z),
                               glm::vec3(0, 0, 1));

            ImGui::Text("Scale");

            glm::vec3 scale = selectedNode->GlobalTransform().Scale();

            ImGui::InputFloat3("##Scale", &scale[0]);

            glm::vec3 scaleDelta = glm::zero<glm::vec3>();

            ImGui::SliderFloat3("##ScaleDelta", &scaleDelta[0], -1, 1);

            scale += scaleDelta;

            if (glm::abs(scale.x) < 0.0001) {
                scale.x = 0.0001;
            }
            if (glm::abs(scale.y) < 0.0001) {
                scale.y = 0.0001;
            }
            if (glm::abs(scale.z) < 0.0001) {
                scale.z = 0.0001;
            }

            selectedNode->GlobalTransform().Scale() = scale;

            ImGui::TreePop();
        }

        AnimationComponent* animationComponent =
            selectedNode->GetObject<AnimationComponent>();
        if (animationComponent != nullptr) {
            if (ImGui::TreeNode("Animation")) {
                for (auto& animation : animationComponent->animations) {
                    if (ImGui::TreeNode(animation.data.name.c_str())) {
                        ImGui::Text("%s", std::format("Duration: {}",
                                                      animation.data.duration)
                                              .c_str());
                        ImGui::Text("%s", std::format("Progress: {}",
                                                      animation.timeActive)
                                              .c_str());
                        ImGui::Checkbox("Playing", &animation.playing);
                        ImGui::Checkbox("Looping", &animation.looping);
                        ImGui::DragFloat("Speed", &animation.speed, 1.0f, 0.0f,
                                         5.0f, "%.2f");
                        // animation.data.tracks.front().inputs add this maybe
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            };
        }

        int index = 0;
        for (GameObject* obj : selectedNode->AttachedObjects()) {
            ImGui::PushID(obj->GetID());
            if (ImGui::TreeNode(
                    std::format("{}: {}", index, obj->GetName()).c_str())) {
                ImGui::Text("Object ID: %i", obj->GetID());

                bool objEnabled = obj->IsEnabled();

                ImGui::Checkbox("Enabled", &objEnabled);

                obj->SetEnabled(objEnabled);

                ImGuiDrawable* imguiObj = dynamic_cast<ImGuiDrawable*>(obj);

                if (imguiObj) {
                    ImGui::Separator();

                    imguiObj->DrawImGui();
                }
                ImGui::TreePop();
            }
            index++;
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void DrawFiles() {
    ImGui::Begin("Files");
    ImGui::End();
}

void HandleMousePicking(Scene& scene, float resX, float resY) {
    if (mainCamera != nullptr && ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!ImGuizmo::IsOver() && !ImViewGuizmo::IsOver()) {
            ImVec2 mousePosition = ImGui::GetMousePos();
            ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();

            float mouseX = mousePosition.x - cursorScreenPosition.x;
            float mouseY = mousePosition.y - cursorScreenPosition.y;

            if (mouseX >= 0.0f && mouseX <= resX && mouseY >= 0.0f &&
                mouseY <= resY) {
                float ndcX = (2.0f * mouseX) / resX - 1.0f;
                float ndcY = 1.0f - (2.0f * mouseY) / resY;

                // Move somewhere else
                mainCamera->SetAspectRatio(resX / resY);

                glm::mat4 projection = mainCamera->ProjectionMatrix();
                glm::mat4 view = mainCamera->ViewMatrix();

                glm::vec4 clipSpacePosition(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 viewSpacePosition =
                    glm::inverse(projection) * clipSpacePosition;
                viewSpacePosition.z = -1.0f;
                viewSpacePosition.w = 0.0f;

                glm::vec3 rayDirection = glm::normalize(
                    glm::vec3(glm::inverse(view) * viewSpacePosition));
                glm::vec3 rayOrigin =
                    mainCamera->GlobalTransform().Position().Value();

                bool hitSomething = false;

                if (Physics::System* physicsSystem =
                        scene.GetComponent<Physics::System>()) {
                    float maxDistance = 1000.0f;
                    glm::vec3 ray = rayDirection * maxDistance;

                    SceneNode* hitNode = physicsSystem->CastRay(rayOrigin, ray);
                    if (hitNode != nullptr) {
                        selectedNode = hitNode;
                        hitSomething = true;
                    }
                }

                if (!hitSomething) {
                    // Add Bounds fallback

                    selectedNode = nullptr;
                }
            }
        }
    }
}

void DrawSceneView(Scene& scene) {
    ImGui::SetNextWindowSize(ImVec2(1024, 576), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::RadioButton("Translate",
                               currentGizmoOperation == ImGuizmo::TRANSLATE)) {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate",
                               currentGizmoOperation == ImGuizmo::ROTATE)) {
            currentGizmoOperation = ImGuizmo::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale",
                               currentGizmoOperation == ImGuizmo::SCALE)) {
            currentGizmoOperation = ImGuizmo::SCALE;
        }
        ImGui::EndMenuBar();
    }

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    float resX = std::max(1.0f, viewportSize.x);
    float resY = std::max(1.0f, viewportSize.y);

    Editor::HandleMousePicking(scene, resX, resY);

    scene.GetGraphics()->UpdateScreenResolution(glm::vec2(resX, resY));
    scene.GetGraphics()->GetMainFramebuffer()->SetSize(glm::uvec2(resX, resY));

    // ImGui::ShowDemoWindow();

    Time::Update();
    scene.Update();
    scene.Render();

    GLuint textureID = scene.GetGraphics()
                           ->GetMainFramebuffer()
                           ->GetColorTexture()
                           ->GetHandle();

    ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();

    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(resX, resY),
                 ImVec2(0, 1), ImVec2(1, 0));

    if (mainCamera != nullptr) {
        if (selectedNode != nullptr) {
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(cursorScreenPosition.x, cursorScreenPosition.y,
                              resX, resY);

            glm::mat4 cameraView = mainCamera->ViewMatrix();
            glm::mat4 cameraProjection = mainCamera->ProjectionMatrix();
            glm::mat4 nodeTransform = selectedNode->GlobalTransform().Value();

            ImGuizmo::Manipulate(glm::value_ptr(cameraView),
                                 glm::value_ptr(cameraProjection),
                                 currentGizmoOperation, ImGuizmo::WORLD,
                                 glm::value_ptr(nodeTransform));

            if (ImGuizmo::IsUsing()) {
                selectedNode->GlobalTransform() = nodeTransform;
            }
        }

        ImViewGuizmo::Style& viewStyle = ImViewGuizmo::GetStyle();
        viewStyle.scale = 0.65f;
        viewStyle.bigCircleColor = IM_COL32(30, 30, 30, 120);

        glm::vec3 cameraPosition = mainCamera->GlobalTransform().Position();
        glm::quat cameraRotation = mainCamera->GlobalTransform().Rotation();

        glm::vec3 pivot =
            (selectedNode != nullptr)
                ? selectedNode->GlobalTransform().Position().Value()
                : glm::vec3(0.0f);

        float gizmoRadius = 128.0f * viewStyle.scale;
        ImVec2 viewGizmoCenter =
            ImVec2(cursorScreenPosition.x + resX - gizmoRadius - 2.0f,
                   cursorScreenPosition.y + gizmoRadius + 2.0f);

        if (ImViewGuizmo::Rotate(cameraPosition, cameraRotation, pivot,
                                 viewGizmoCenter)) {
            mainCamera->GlobalTransform().Position() = cameraPosition;
            mainCamera->GlobalTransform().Rotation() = cameraRotation;
        }

        float toolButtonSize = viewStyle.toolButtonRadius * viewStyle.scale;
        float spacing = 10.0f;

        ImVec2 panPosition = ImVec2(
            viewGizmoCenter.x - (toolButtonSize * 2.0f) - (spacing / 2.0f),
            viewGizmoCenter.y + gizmoRadius + spacing);

        ImVec2 dollyPosition =
            ImVec2(viewGizmoCenter.x + (spacing / 2.0f),
                   viewGizmoCenter.y + gizmoRadius + spacing);
        if (ImViewGuizmo::Pan(cameraPosition, cameraRotation, panPosition,
                              0.05f)) {
            mainCamera->GlobalTransform().Position() = cameraPosition;
        }

        if (ImViewGuizmo::Dolly(cameraPosition, cameraRotation, dollyPosition,
                                0.2f)) {
            mainCamera->GlobalTransform().Position() = cameraPosition;
        }
    }

    ImGui::End();
}

void MainLoop() {
    // temporary
    std::unique_ptr<Scene> scene(Scene::CreateStandaloneScene());
    InitScene(*scene);

    scene->GetGraphics()->UpdateScreenResolution(glm::vec2(1024.0f, 576.0f));

    bool shouldClose = false;
    while (!shouldClose) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                shouldClose = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                shouldClose = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        ImViewGuizmo::BeginFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        Editor::DrawMainMenuBar(shouldClose);
        Editor::DrawGraph(*scene);
        Editor::DrawInspector();
        Editor::DrawFiles();
        Editor::DrawSceneView(*scene);

        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}
} // namespace Editor
