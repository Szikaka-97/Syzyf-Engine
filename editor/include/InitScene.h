#pragma once

#include "GltfImporter.h"
#include <AiNode.h>
#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
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
#include <glm/geometric.hpp>
#include <physics/Body.h>
#include <physics/DebugRenderer.h>
#include <physics/System.h>
#include <physics/Water.h>

#include <imgui.h>

class EditorCameraTag : public GameObject {};

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

inline void InitScene(Scene& mainScene, Camera*& mainCamera) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<Physics::DebugRenderer>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<AnimationSystem>();

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

    Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
        "./res/textures/citrus_orchard_road_puresky.hdr",
        Texture::HDRColorBuffer);
    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

    Material* skyMat = new Material(skyProg);
    skyMat->SetValue("skyboxTexture", skyCubemap);

    SceneNode* playerNode = mainScene.CreateNode("Player");
    playerNode->GlobalTransform().Position() = glm::vec3(0.0f, 1.5f, 0.0f);
    playerNode->AddObject<Mover>();
    // MOVE OUTSIDE OF HERE
    mainCamera = playerNode->AddObject<Camera>(
        Camera::Perspective(40.0f, 16.0f / 9.0f, 0.5f, 200.0f));
    mainCamera->AddObject<EditorCameraTag>();
    playerNode->AddObject<Bloom>();
    playerNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    playerNode->AddObject<ColorGrading>();

    auto floorNode =
        GltfImporter::LoadScene(&mainScene, "./res/models/floor.glb");
    floorNode->AddObject<Skybox>(skyMat);
    MeshRenderer* floorMeshRenderer =
        floorNode->GetObjectInChildren<MeshRenderer>();
    floorMeshRenderer->GetNode()->AddObject<Physics::Body>(Physics::Body::Mesh(
        floorMeshRenderer->GetMesh(), JPH::EMotionType::Static,
        Physics::Layers::NON_MOVING));
    floorNode->AddObject<Surface>(floorMeshRenderer->GetMesh(), 1.0f);

    auto lightNode = mainScene.CreateNode("Point Light");
    lightNode->AddObject<Light>(Light::PointLight({1, 1, 1}, 10, 1))
        ->SetShadowCasting(true);
    lightNode->GlobalTransform().Position() = {-1, 2.2f, 0};

    auto lightNode2 = mainScene.CreateNode("Directional Light");
    lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 1))
        ->SetShadowCasting(true);
    lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
    lightNode2->GlobalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(64.0f, 0.0f, 0.0f)));

    auto envProbe2 = mainScene.CreateNode("Reflection Probe");
    envProbe2->AddObject<ReflectionProbe>();
    envProbe2->GlobalTransform().Position() = {-10.0f, 1.5f, 0.6f};

    auto envProbe3 = mainScene.CreateNode("Reflection Probe");
    envProbe3->AddObject<ReflectionProbe>();
    envProbe3->GlobalTransform().Position() = {-29.0f, 1.5f, 0.6f};

    SceneNode* skeletonNode = GltfImporter::LoadScene(
        &mainScene, "./res/models/szkielet6.glb", "Szkielet");
    skeletonNode->GlobalTransform().Scale() = glm::vec3(0.2f);

    SceneNode* skeleton2Node = GltfImporter::LoadScene(
        &mainScene, "./res/models/szkielet6.glb", "Szkielet2");
    skeleton2Node->GlobalTransform().Position() = {0.0f, 0.0f, 5.0f};
    skeleton2Node->GlobalTransform().Scale() = glm::vec3(0.2f);

    SceneNode* bimberman = GltfImporter::LoadScene(
        &mainScene, "./res/models/bimbermann.glb", "Bimberman");
    bimberman->GlobalTransform().Position() = {0.0f, 0.0f, 10.0f};
}
