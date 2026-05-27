#pragma once

#include <functional>

#include <GameObject.h>
#include <MeshRenderer.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "game_scripts/AttackEffects/EffectBase.h"
#include "physics/Body.h"
#include "physics/ICollisionReceiver.h"

#include "game_scripts/AttackEffects/EffectsManager.h"
#include <Scene.h>
#include <Resources.h>

class ThrowableObject : public GameObject, public Physics::ICollisionReceiver {
public:
    using EffectFactory = std::function<EffectBase*(SceneNode*)>;

    template <typename TEffect>
void SetEffect(std::function<void(TEffect*)> configure = nullptr) {
    m_EffectFactory = [this, configure](SceneNode* node) -> EffectBase* {
        TEffect* effect = node->AddObject<TEffect>();
        if (configure) configure(effect);
        effect->Init();

        ShaderProgram* pbrProg = ShaderProgram::Build()
                                     .WithVertexShader("./res/shaders/lit.vert")
                                     .WithPixelShader("./res/shaders/pbr.frag")
                                     .Link();

        Scene* scene = this->GetScene(); 
        Texture2D* reflectiveDiffuse = scene->Resources()->Get<Texture2D>(
            "./res/textures/material_preview/worn-shiny-metal-albedo.png",
            Texture::ColorTextureRGB);
        Texture2D* reflectiveNormal = scene->Resources()->Get<Texture2D>(
            "./res/textures/material_preview/worn-shiny-metal-Normal-ogl.png",
            Texture::TechnicalMapXYZ);
        Texture2D* reflectiveARM = scene->Resources()->Get<Texture2D>(
            "./res/textures/material_preview/worn-shiny-metal-arm.png",
            Texture::TechnicalMapXYZ);

        Material* reflectiveMat = new Material(pbrProg);
        reflectiveMat->SetValue("albedoMap", reflectiveDiffuse);
        reflectiveMat->SetValue("normalMap", reflectiveNormal);
        reflectiveMat->SetValue("armMap", reflectiveARM);

        Mesh* effectMesh = scene->Resources()->Get<Mesh>("./res/models/not_cube.obj");

        effect->SetEffectRenderer(effectMesh, reflectiveMat);
        return effect;
    };
}

    void SetEffectFactory(EffectFactory factory) {
        m_EffectFactory = std::move(factory);
    }

    void SetVisual(Mesh* mesh, Material* mat) {
        m_VisualMesh = mesh;
        m_VisualMat  = mat;
    }

    float& MaxLifetime() { return m_MaxLifetime; }


    void Awake() {
        if (m_VisualMesh && m_VisualMat)
            GetNode()->AddObject<MeshRenderer>(m_VisualMesh, m_VisualMat);
    }

    void Update() {
        if (m_DeletionCountdown > 0) {
            m_DeletionCountdown--;
            if (m_DeletionCountdown == 0) {
                DetachChildrenAndDeleteSelf();
            }
            return;
        }

        if (m_ShouldSpawn) {
            m_ShouldSpawn = false;

            SpawnEffect();

            if (auto* body = GetNode()->GetObject<Physics::Body>()) {
                body->SetLinearVelocity(glm::vec3(0.0f));
                body->SetAngularVelocity(glm::vec3(0.0f));
            }

            m_DeletionCountdown = 2;
            return;
        }

        m_ElapsedTime += Time::Delta();
        if (m_ElapsedTime >= m_MaxLifetime) {
            spdlog::warn("ThrowableObject: lifetime expired, spawning effect.");
            m_ImpactPos   = GetNode()->GlobalTransform().Position();
            m_ShouldSpawn = true;
        }
    }


    void OnCollisionEnter(SceneNode* other) override {
        if (m_ShouldSpawn || m_DeletionCountdown > 0) return;
        if (other == GetNode())                         return;

        spdlog::debug("ThrowableObject: hit \"{}\"",
                      other ? other->GetName() : "<null>");

        m_ImpactPos   = GetNode()->GlobalTransform().Position();
        m_ShouldSpawn = true;
    }

    void OnCollisionStay(SceneNode* /*other*/)  {}
    void OnCollisionExit(SceneNode* /*other*/) override {}

private:
    void DetachChildrenAndDeleteSelf() {
        Scene*     scene  = GetScene();
        SceneNode* myNode = GetNode();
        SceneNode* root   = scene->GetRootNode();

        for (SceneNode* child : myNode->GetChildren()) {
            child->SetParent(root);  
        }
        myNode->SetParent(root);

        scene->QueueDelete(myNode);

    }

    void SpawnEffect() {
        if (!m_EffectFactory) {
            spdlog::warn("ThrowableObject: no EffectFactory set — nothing spawned.");
            return;
        }

        Scene* scene = GetScene();

        SceneNode* effectNode = scene->CreateNode("ThrowableEffect");
        effectNode->GlobalTransform().Position() = m_ImpactPos;

        EffectBase* effect = m_EffectFactory(effectNode);

        if (effect) {
            spdlog::info(
                "ThrowableObject: effect spawned at ({:.2f},{:.2f},{:.2f})",
                m_ImpactPos.x, m_ImpactPos.y, m_ImpactPos.z);
        } else {
            spdlog::error("ThrowableObject: EffectFactory returned null.");
            scene->QueueDelete(effectNode);
        }
    }


    EffectFactory  m_EffectFactory;

    bool           m_ShouldSpawn      = false;
    int            m_DeletionCountdown = 0;

    glm::vec3      m_ImpactPos        = {};

    Mesh*          m_VisualMesh        = nullptr;
    Material*      m_VisualMat         = nullptr;

    float          m_MaxLifetime       = 10.0f;
    float          m_ElapsedTime       = 0.0f;
};