#include "game_scripts/AttackEffects/EffectBase.h"
#include "game_scripts/enemies/EnemyBullet.h"
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>
#include "game_scripts/AttackEffects/EffectsManager.h"
#include "game_scripts/FireParticles.h"

void EffectFire::OnApplySpecials() {
    if (special1) damage *= modifier;
    if (special2) dotRemainingTime *= static_cast<float>(modifier);
    spdlog::info("FireEffect: radius={:.1f}, damage={:.0f}, burnDur={:.1f}s",
                 radius, damage, dotRemainingTime);
}
void EffectFire::OnApplyToEnemy(EnemyBase* enemy) {
    enemy->ApplyBurn(damage, dotRemainingTime, timeInterval);
}


void EffectPetrify::OnApplySpecials() {
    if (special1) petrifyRemainingTime *= static_cast<float>(modifier);
    if (special2) radius *= static_cast<float>(modifier);
    spdlog::info("PetrifyEffect: radius={:.1f}, freezeDur={:.1f}s",
                 radius, petrifyRemainingTime);
}
void EffectPetrify::OnApplyToEnemy(EnemyBase* enemy) {
    if (enemy->IsPetrified()) return;
    float slowFactor = (ingredientCount == 1) ? 0.5f : 0.0f;
    enemy->ApplyPetrify(slowFactor, petrifyRemainingTime);
}

void EffectTornado::OnApplySpecials() {
    if (special1) radius               *= static_cast<float>(modifier);
    if (special2) tornadoRemainingTime *= static_cast<float>(modifier);
    spdlog::info("TornadoEffect: radius={:.1f}, duration={:.1f}s", radius, tornadoRemainingTime);
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
        if (ingredientCount == 1)
            GetScene()->QueueDelete(bullet->myNode);
        else
            bullet->BulletInTornadoAction(myNode, radius, rotationSpeed);
    }
}

void EffectConfuse::OnApplySpecials() {
    if (special1) confuseRemainingTime *= static_cast<float>(modifier);
    if (special2) damage *= modifier;
    spdlog::info("ConfuseEffect: radius={:.1f}, confuseDur={:.1f}s", radius, confuseRemainingTime);
}
void EffectConfuse::OnApplyToEnemy(EnemyBase* enemy) {
    bool attackPrecisely = (ingredientCount == 2);
    enemy->ApplyConfuse(confuseRemainingTime, attackPrecisely);
}

void EffectExplosion::OnInit() {


    SceneNode* explosionModel =
            ResourceDatabase::Global
    ->Get<GltfScene>("./res/models/effects/explode1.glb")
->Instantiate(GetScene(), GetNode(), "explosion effect");
    explosionModel->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    explosionModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
}

void EffectExplosion::OnApplySpecials() {
    speed = 1.0f;

    spdlog::info("EffectExplosion: range={:.1f}, damage={:.1f}, duration={:.1f}s",
                 GetRange(), GetDamage(), explosionDuration);
}

void EffectExplosion::OnApplyToEnemy(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));
}