#pragma once
#include <glm/glm.hpp>
#include <GameObject.h>
#include <MeshRenderer.h>
#include <Shader.h>
#include <Material.h>
#include <GltfScene.h>

class SceneNode;
class Scene;

class LootItem : public GameObject{
public:
    virtual ~LootItem() = default;
    virtual void Spawn(Scene* scene, const glm::vec3& position) const = 0;
};

class LootBone : public LootItem {
    public:
        void Spawn(Scene* scene, const glm::vec3& position) const override{
            SceneNode* node = scene->Resources()->Get<GltfScene>("./res/models/ingredients/bone.glb")->Instantiate(scene, nullptr, "bone");
            node->GlobalTransform().Position() = position;
    }
};

class LootPotato : public LootItem {
    public:
    void Spawn(Scene* scene, const glm::vec3& position) const override {
            auto* node = scene->CreateNode("LootPotato");
    node->GlobalTransform().Position() = position;

    ShaderProgram* pbrProg = ShaderProgram::Build()
                .WithVertexShader("./res/shaders/lit.vert")
                .WithPixelShader("./res/shaders/pbr.frag")
                .Link();


            //Scene* scene = this->GetScene();   
            Texture2D* albedo = scene->Resources()->Get<Texture2D>(
                "./res/textures/material_preview/worn-shiny-metal-albedo.png",
                Texture::ColorTextureRGB);
            Texture2D* normal = scene->Resources()->Get<Texture2D>(
                "./res/textures/material_preview/worn-shiny-metal-Normal-ogl.png",
                Texture::TechnicalMapXYZ);
            Texture2D* arm = scene->Resources()->Get<Texture2D>(
                "./res/textures/material_preview/worn-shiny-metal-arm.png",
                Texture::TechnicalMapXYZ);

            Material* mat = new Material(pbrProg);
            mat->SetValue("albedoMap", albedo);
            mat->SetValue("normalMap", normal);
            mat->SetValue("armMap",    arm);

            Mesh* mesh = scene->Resources()->Get<Mesh>("./res/models/jake_tangents.glb");

     node->AddObject<MeshRenderer>(mesh, mat);
    // node->AddObject<PickupScript>();
    }
};

class LootBeetroot : public LootItem {
    public:
    void Spawn(Scene* scene, const glm::vec3& position) const {
            auto* node = scene->CreateNode("LootBeetroot");
    node->GlobalTransform().Position() = position;
    // node->AddObject<MeshRenderer>(potionMesh, potionMaterial);
    // node->AddObject<PickupScript>();
    }
};

class LootCrystal : public LootItem {
    public:
    void Spawn(Scene* scene, const glm::vec3& position) const {
            auto* node = scene->CreateNode("LootCrystal");
    node->GlobalTransform().Position() = position;
    // node->AddObject<MeshRenderer>(potionMesh, potionMaterial);
    // node->AddObject<PickupScript>();
    }
};