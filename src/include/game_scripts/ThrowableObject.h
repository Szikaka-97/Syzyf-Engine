#pragma once

#include <functional>

#include <GameObject.h>
#include <MeshRenderer.h>
#include <Scene.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "game_scripts/AttackEffects/EffectBase.h"
#include "game_scripts/AttackEffects/combos/ComboEffectBase.h"
#include "physics/Body.h"
#include "physics/ICollisionReceiver.h"
#include "game_scripts/AttackEffects/EffectsManager.h"
#include <game_scripts/enemies/EnemyBase.h>
#include <Scene.h>
#include <Resources.h>

class ThrowableObject : public GameObject, public Physics::ICollisionReceiver {
public:
    using EffectFactory = std::function<EffectBase*(SceneNode*)>;

    using ComboFactory = std::function<void(SceneNode*)>;

    template <typename TEffect>
    void SetEffect(std::function<void(TEffect*)> configure = nullptr) {
        m_ComboFactory  = nullptr;
        m_EffectFactory = [this, configure](SceneNode* node) -> EffectBase* {
            TEffect* effect = node->AddObject<TEffect>();
            if (configure) configure(effect);
            effect->Init();

            ShaderProgram* pbrProg = ShaderProgram::Build()
                .WithVertexShader("./res/shaders/lit.vert")
                .WithPixelShader("./res/shaders/pbr.frag")
                .Link();

            if (!pbrProg) {
                spdlog::error("ThrowableObject: shader nie skompilował się");
                return effect;
            }

            Scene* scene = this->GetScene();
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
            Mesh* effectMesh = scene->Resources()->Get<Mesh>("./res/models/not_cube.obj");

            effect->SetEffectRenderer(effectMesh, mat);
            return effect;
        };
    }

    void SetEffectFactory(EffectFactory factory) {
        m_ComboFactory  = nullptr;
        m_EffectFactory = std::move(factory);
    }

    template <typename TCombo>
    void SetComboEffect(std::function<void(TCombo*)> configure = nullptr) {
        m_EffectFactory = nullptr;
        m_ComboFactory = [this,configure](SceneNode* node) {
            TCombo* combo = node->AddObject<TCombo>();
            if (configure) configure(combo);
            ShaderProgram* pbrProg = ShaderProgram::Build()
                .WithVertexShader("./res/shaders/lit.vert")
                .WithPixelShader("./res/shaders/pbr.frag")
                .Link();

            if (!pbrProg) {
                spdlog::error("ThrowableObject: shader nie skompilował się");
            }

            Scene* scene = this->GetScene();
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

            Mesh* mesh = scene->Resources()->Get<Mesh>("./res/models/effectBase.glb");
            combo->SetEffectRenderer(mesh, mat);
        };
    }


    void SetComboFactory(ComboFactory factory) {
        m_EffectFactory = nullptr;
        m_ComboFactory  = std::move(factory);
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

        if (CheckEnemyHit()) {
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
        if (other == GetNode()) return;

        spdlog::debug("ThrowableObject: hit \"{}\"",
                      other ? other->GetName() : "<null>");

        if (other != nullptr) {
            if (EnemyBase* enemy = FindEnemyOnNodeOrParents(other)) {
                KillEnemy(enemy);
                return;
            }
        }

        m_ImpactPos   = GetNode()->GlobalTransform().Position();
        m_ShouldSpawn = true;
    }

    void OnCollisionStay(SceneNode* /*other*/)  {}
    void OnCollisionExit(SceneNode* /*other*/) override {}

private:
    EnemyBase* FindEnemyOnNodeOrParents(SceneNode* node) {
        SceneNode* current = node;

        while (current != nullptr) {
            if (EnemyBase* enemy = current->GetObject<EnemyBase>()) {
                return enemy;
            }

            current = current->GetParent();
        }

        return nullptr;
    }

    bool CheckEnemyHit() {
        if (m_ShouldSpawn || m_DeletionCountdown > 0) {
            return false;
        }

        glm::vec3 bottlePos = GetNode()->GlobalTransform().Position();

        for (EnemyBase* enemy : GetScene()->FindObjectsOfType<EnemyBase>()) {
            if (enemy == nullptr || enemy->myNode == nullptr) {
                continue;
            }

            glm::vec3 enemyPos = enemy->myNode->GlobalTransform().Position();

            if (glm::distance(bottlePos, enemyPos) <= 0.9f) {
                KillEnemy(enemy);
                return true;
            }
        }

        return false;
    }

    void KillEnemy(EnemyBase* enemy) {
        if (enemy == nullptr || enemy->myNode == nullptr) {
            return;
        }

        m_ImpactPos = GetNode()->GlobalTransform().Position();

        spdlog::info(
            "ThrowableObject: bottle killed enemy \"{}\"",
            enemy->myNode->GetName()
        );

        enemy->TakeDamage(999);
        m_ShouldSpawn = true;
    }

    void SpawnEffect() {
        Scene*     scene      = GetScene();
        SceneNode* effectNode = scene->CreateNode("ThrowableEffect");
        effectNode->GlobalTransform().Position() = m_ImpactPos;

        if (m_EffectFactory) {
            EffectBase* effect = m_EffectFactory(effectNode);
            if (effect) {
                spdlog::info("ThrowableObject: EffectBase spawned at "
                             "({:.2f},{:.2f},{:.2f})",
                             m_ImpactPos.x, m_ImpactPos.y, m_ImpactPos.z);
            } else {
                spdlog::error("ThrowableObject: EffectFactory zwróciła null.");
                scene->QueueDelete(effectNode);
            }
        }
        else if (m_ComboFactory) {
            m_ComboFactory(effectNode);

            if (effectNode->AttachedObjects().empty()) {
                spdlog::error("ThrowableObject: ComboFactory nie dodała obiektu. "
                              "Sprawdź czy configure wywołuje Init(e1, range, dmg, dur).");
                scene->QueueDelete(effectNode);
            } else {
                spdlog::info("ThrowableObject: ComboEffectBase spawned at "
                             "({:.2f},{:.2f},{:.2f})",
                             m_ImpactPos.x, m_ImpactPos.y, m_ImpactPos.z);
            }
        }
        else {
            spdlog::warn("ThrowableObject: brak fabryki — "
                         "wywołaj SetEffect<T>() lub SetComboEffect<T>().");
            scene->QueueDelete(effectNode);
        }
    }

    void DetachChildrenAndDeleteSelf() {
        Scene*     scene  = GetScene();
        SceneNode* myNode = GetNode();
        SceneNode* root   = scene->GetRootNode();

        for (SceneNode* child : myNode->GetChildren())
            child->SetParent(root);

        myNode->SetParent(root);
        scene->QueueDelete(myNode);
    }

    EffectFactory m_EffectFactory;
    ComboFactory  m_ComboFactory;

    bool      m_ShouldSpawn       = false;
    int       m_DeletionCountdown = 0;
    glm::vec3 m_ImpactPos         = {};

    Mesh*     m_VisualMesh  = nullptr;
    Material* m_VisualMat   = nullptr;
    float     m_MaxLifetime = 10.0f;
    float     m_ElapsedTime = 0.0f;
};