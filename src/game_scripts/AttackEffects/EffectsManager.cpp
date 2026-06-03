

#include "./include/game_scripts/AttackEffects/EffectBase.h"
#include "./include/game_scripts/enemies/EnemyBullet.h"
#include "GltfScene.h"
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include "./include/game_scripts/AttackEffects/EffectsManager.h"

EffectFire::EffectFire() = default;
EffectPetrify::EffectPetrify() = default;
EffectTornado::EffectTornado() = default;
EffectConfuse::EffectConfuse() = default;
EffectExplosion::EffectExplosion() = default;

void EffectFire::OnInit() {
    /*ShaderProgram* pbrProg = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/lit.vert")
        .WithPixelShader("./res/shaders/pbr.frag")
        .Link();

    if (!pbrProg) {
        spdlog::error("EffectFire: shader nie skompilowa³ siê");
        return;
    }

    Scene* scene = this->GetScene();
    Texture2D* albedo = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/fire1_diffuse.png",
        Texture::ColorTextureRGB);
    Texture2D* normal = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/fire1_normal.png",
        Texture::TechnicalMapXYZ);

    Material* mat = new Material(pbrProg);
    mat->SetValue("albedoMap", albedo);
    mat->SetValue("normalMap", normal);

    Mesh* mesh = scene->Resources()->Get<Mesh>("./res/models/effects/fire1.glb");
    SetEffectRenderer(mesh, mat);*/

    ResourceDatabase::Global->Get<GltfScene>("./res/models/effects/fire1.glb")->Instantiate(GetScene(), GetNode(), "fire effect");
    AnimationComponent* anim = GetNode()->GetObjectInChildren<AnimationComponent>();
    if (anim) {
        spdlog::info("EffectFire::OnInit: znaleziono AnimationComponent, odtwarzanie animacji");
        anim->animations[0].looping = true;
        anim->Play("ArmatureAction"); //ArmatureAction
    } else {
        spdlog::warn("EffectFire::OnInit: brak AnimationComponent w wêŸle");
    }
}
void EffectFire::OnApplySpecials() {
    if (special1) damage *= modifier;
    if (special2) dotRemainingTime *= static_cast<float>(modifier);

    spdlog::info("FireEffect: radius={:.1f}, damage={:.0f}, burnDur={:.1f}s",
                 radius, damage, dotRemainingTime);
}

void EffectFire::OnApplyToEnemy(EnemyBase* enemy) {
    enemy->ApplyBurn(damage, dotRemainingTime, timeInterval);
    spdlog::debug("FireEffect: burned enemy (dmg={:.0f}/{}s, dur={:.1f}s)",
                  damage, timeInterval, dotRemainingTime);
}

void EffectPetrify::OnInit() {
    ShaderProgram* pbrProg = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/lit.vert")
        .WithPixelShader("./res/shaders/pbr.frag")
        .Link();

    if (!pbrProg) {
        spdlog::error("EffectPetrify: shader nie skompilowa³ siê");
        return;
    }

    Scene* scene = this->GetScene();
    Texture2D* albedo = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/petrify1_diffuse.png",
        Texture::ColorTextureRGB);
    Texture2D* normal = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/petrify1_normal.png",
        Texture::TechnicalMapXYZ);

    Material* mat = new Material(pbrProg);
    mat->SetValue("albedoMap", albedo);
    mat->SetValue("normalMap", normal);

    Mesh* mesh = scene->Resources()->Get<Mesh>("./res/models/effects/petrify1.glb");
    SetEffectRenderer(mesh, mat);
}

void EffectPetrify::OnApplySpecials() {
    if (special1) petrifyRemainingTime *= static_cast<float>(modifier);
    if (special2) radius              *= static_cast<float>(modifier);

    spdlog::info("PetrifyEffect: radius={:.1f}, freezeDur={:.1f}s, ingredients={}",
                 radius, petrifyRemainingTime, ingredientCount);
}

void EffectPetrify::OnApplyToEnemy(EnemyBase* enemy) {
    if (enemy->IsPetrified()) return;

    float slowFactor = (ingredientCount == 1) ? 0.5f : 0.0f;
    enemy->ApplyPetrify(slowFactor, petrifyRemainingTime);

    spdlog::debug("PetrifyEffect: petrified enemy (slowFactor={:.2f}, dur={:.1f}s)",
                  slowFactor, petrifyRemainingTime);
}

void EffectTornado::OnInit() {
    ShaderProgram* pbrProg = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/lit.vert")
        .WithPixelShader("./res/shaders/pbr.frag")
        .Link();

    if (!pbrProg) {
        spdlog::error("EffectTornado: shader nie skompilowa³ siê");
        return;
    }

    Scene* scene = this->GetScene();
    Texture2D* albedo = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/tornado1_diffuse.png",
        Texture::ColorTextureRGB);
    Texture2D* normal = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/tornado1_normal.png",
        Texture::TechnicalMapXYZ);

    Material* mat = new Material(pbrProg);
    mat->SetValue("albedoMap", albedo);
    mat->SetValue("normalMap", normal);

    Mesh* mesh = scene->Resources()->Get<Mesh>("./res/models/effects/tornado1.glb");
    SetEffectRenderer(mesh, mat);
}

void EffectTornado::OnApplySpecials() {
    if (special1) radius               *= static_cast<float>(modifier);
    if (special2) tornadoRemainingTime *= static_cast<float>(modifier);

    spdlog::info("TornadoEffect: radius={:.1f}, duration={:.1f}s, ingredients={}",
                 radius, tornadoRemainingTime, ingredientCount);
}

void EffectTornado::OnUpdate() {
    if (myNode) {
        glm::quat delta = glm::angleAxis(
            glm::radians(rotationSpeed * Time::Delta()),
            glm::vec3(0.0f, 1.0f, 0.0f));
        myNode->GlobalTransform().Rotation() =
            delta * glm::quat(myNode->GlobalTransform().Rotation());
    }

    m_DamageTimer += Time::Delta();
    if (m_DamageTimer >= damageInterval) {
        m_DamageTimer = 0.0f;
        for (auto* enemy : ScanEnemiesInRadius())
            enemy->TakeDamage(static_cast<int>(damage));
    }

    ScanAndHandleBullets();
}

void EffectTornado::ScanAndHandleBullets() {
    glm::vec3 pos = GetPosition();

    for (auto* bullet : GetScene()->FindObjectsOfType<EnemyBullet>()) {
        if (!bullet->myNode) continue;
        glm::vec3 bpos = bullet->myNode->GlobalTransform().Position();
        if (glm::distance(pos, bpos) > radius) continue;

        if (ingredientCount == 1) {
            GetScene()->QueueDelete(bullet->myNode);
        } else {
            bullet->BulletInTornadoAction(myNode, radius, rotationSpeed);
        }
    }
}
void EffectConfuse::OnInit() {
    ShaderProgram* pbrProg = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/lit.vert")
        .WithPixelShader("./res/shaders/pbr.frag")
        .Link();

    if (!pbrProg) {
        spdlog::error("EffectConfuse: shader nie skompilowa³ siê");
        return;
    }

    Scene* scene = this->GetScene();
    Texture2D* albedo = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/confuse1_diffuse.png",
        Texture::ColorTextureRGB);
    Texture2D* normal = scene->Resources()->Get<Texture2D>(
        "./res/textures/effects/confuse1_normal.png",
        Texture::TechnicalMapXYZ);

    Material* mat = new Material(pbrProg);
    mat->SetValue("albedoMap", albedo);
    mat->SetValue("normalMap", normal);

    Mesh* mesh = scene->Resources()->Get<Mesh>("./res/models/effects/confuse1.glb");
    SetEffectRenderer(mesh, mat);
}


void EffectConfuse::OnApplySpecials() {
    if (special1) confuseRemainingTime *= static_cast<float>(modifier);
    if (special2) damage *= modifier;

    spdlog::info("ConfuseEffect: radius={:.1f}, confuseDur={:.1f}s, ingredients={}",
                 radius, confuseRemainingTime, ingredientCount);
}

void EffectConfuse::OnApplyToEnemy(EnemyBase* enemy) {
    bool attackPrecisely = (ingredientCount == 2);
    enemy->ApplyConfuse(confuseRemainingTime, attackPrecisely);

    spdlog::debug("ConfuseEffect: confused enemy (dur={:.1f}s, attackPrecisely={})",
                  confuseRemainingTime, attackPrecisely);
}

void EffectExplosion::OnInit() {
    ResourceDatabase::Global->Get<GltfScene>("./res/models/effects/explode1.glb")->Instantiate(GetScene(), GetNode(), "fire effect")->GlobalTransform().Position() = GlobalTransform().Position().Value();
}

void EffectExplosion::OnApplySpecials() {
    // Set visual node scale to GetRange() — mirrors:
    //   this.explosionRenderer.transform.localScale = Vector3.one * GetRange()
    if (myNode)
        myNode->GlobalTransform().Scale() = glm::vec3(GetRange());
 
    // Explosion speed in the original is 30 (lifetime += dt * 30)
    speed = 30.0f;
 
    spdlog::info("EffectExplosion: range={:.1f}, damage={:.1f}",
                 GetRange(), GetDamage());
}
 
void EffectExplosion::OnApplyToEnemy(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));
 
    spdlog::debug("EffectExplosion: hit enemy for {:.0f} damage", GetDamage());
}
 
void EffectExplosion::OnUpdate() {
}
