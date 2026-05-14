#include "PotionEffects.h"
#include "EnemyBullet.h"
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

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