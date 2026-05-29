#include "./include/game_scripts/AttackEffects/combos/ComboFirePetrify.h"
#include "./include/game_scripts/AttackEffects/combos/ComboFireTornado.h"
#include "./include/game_scripts/AttackEffects/combos/ComboTornadoPetrify.h"
#include "./include/game_scripts/AttackEffects/combos/ComboExplodePetrify.h"
#include "./include/game_scripts/AttackEffects/combos/ComboExplodeTornado.h"
#include <TimeSystem.h>
#include <spdlog/spdlog.h>

#include "./include/game_scripts/AttackEffects/combos/ComboExplodeConfuse.h"
#include "./include/game_scripts/AttackEffects/combos/ComboPetrifyConfuse.h"
#include "./include/game_scripts/AttackEffects/combos/ComboFireConfuse.h"
#include "./include/game_scripts/AttackEffects/combos/ComboTornadoConfuse.h"
#include "./include/game_scripts//enemies/EnemyBullet.h"

static void RotateNodeY(SceneNode* node, float degPerSec) {
    if (!node) return;
    glm::quat delta = glm::angleAxis(glm::radians(degPerSec * Time::Delta()),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
    node->GlobalTransform().Rotation() =
        delta * glm::quat(node->GlobalTransform().Rotation());
}

static void SetXZScale(SceneNode* node, float radius) {
    if (!node) return;
    glm::vec3 s = node->GlobalTransform().Scale();
    node->GlobalTransform().Scale() = glm::vec3(radius, s.y, radius);
}


// =============================================================================
//  ComboFirePetrify
//  Fire + Petrify — DOT + slow, one-shot per enemy
// =============================================================================
void ComboFirePetrify::ApplyTo(EnemyBase* enemy) {
    float burnDuration = duration * (1.0f - effect2Strength);
    enemy->ApplyBurn(GetDamage(), burnDuration);

    float slowFactor    = 0.5f + 0.5f * (1.0f - effect2Strength);
    float petrifyDur    = duration * (1.0f - effect2Strength);
    enemy->ApplyPetrify(slowFactor, petrifyDur);

    enemy->TakeDamage(static_cast<int>(GetDamage()));
}

void ComboFirePetrify::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }
}


// =============================================================================
//  ComboFireTornado
//  Fire + Tornado — rotating area, DOT on entry, bullet capture
// =============================================================================
void ComboFireTornado::InitTornado() {
    m_TornadoRadius = effect2Strength * maxTornadoRadius;
    SetXZScale(myNode, m_TornadoRadius);
}

void ComboFireTornado::ApplyBurnTo(EnemyBase* enemy) {
    float burnDur = duration * (1.0f - effect2Strength);
    enemy->ApplyBurn(GetDamage(), burnDur);
    enemy->TakeDamage(static_cast<int>(GetDamage()));
}

void ComboFireTornado::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    RotateNodeY(myNode, rotationSpeed);

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyBurnTo(enemy);
    }

    // TODO: scan for EnemyBullet objects within m_TornadoRadius and call
    //       bullet->BulletInTornadoAction(myNode, m_TornadoRadius, rotationSpeed)
}


// =============================================================================
//  ComboTornadoPetrify
//  Tornado + Petrify — rotating area, slow on entry, bullet capture
// =============================================================================
void ComboTornadoPetrify::InitTornado() {
    m_TornadoRadius = effect2Strength * maxTornadoRadius;
    SetXZScale(myNode, m_TornadoRadius);
}

void ComboTornadoPetrify::ApplyPetrifyTo(EnemyBase* enemy) {
    float slowFactor = 0.5f + 0.5f * (1.0f - effect2Strength);
    float petrifyDur = duration * (1.0f - effect2Strength);
    enemy->ApplyPetrify(slowFactor, petrifyDur);
}

void ComboTornadoPetrify::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    RotateNodeY(myNode, rotationSpeed);

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyPetrifyTo(enemy);
    }

    // TODO: EnemyBullet capture (see note at top of file)
}


// =============================================================================
//  ComboExplodePetrify
//  Explode + Petrify — expanding radius, instant damage + slow, one-shot
// =============================================================================
void ComboExplodePetrify::ApplyTo(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));

    float slowFactor = 0.5f + 0.5f * (1.0f - effect2Strength);
    float petrifyDur = duration * (1.0f - effect2Strength);
    enemy->ApplyPetrify(slowFactor, petrifyDur);
}

void ComboExplodePetrify::Update() {
    if (myNode) {
        glm::vec3 s = myNode->GlobalTransform().Scale();
        s.x += Time::Delta();
        s.z += Time::Delta();
        myNode->GlobalTransform().Scale() = s;
    }

    ComboEffectBase::Update();
    if (m_Expired) return;

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }
}


// =============================================================================
//  ComboExplodeTornado
//  Explode + Tornado — rotating area, instant damage on entry, bullet capture
// =============================================================================
void ComboExplodeTornado::InitTornado() {
    m_TornadoRadius = effect2Strength * maxTornadoRadius;
    SetXZScale(myNode, m_TornadoRadius);
}

void ComboExplodeTornado::ApplyTo(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));
}

void ComboExplodeTornado::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    RotateNodeY(myNode, rotationSpeed);

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }

    // TODO: EnemyBullet capture (see note at top of file)
}

static bool PreciseFromCount(int count) { return count == 2; }

void ComboExplodeConfuse::ApplyTo(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));

    float confuseDur = duration * effect2Strength;
    enemy->ApplyConfuse(confuseDur, PreciseFromCount(ingredientCount));

    spdlog::debug("ComboExplodeConfuse: hit enemy — dmg={:.0f}, confuseDur={:.1f}s",
                  GetDamage(), confuseDur);
}

void ComboExplodeConfuse::Update() {
    if (myNode) {
        glm::vec3 s = myNode->GlobalTransform().Scale();
        s.x += Time::Delta();
        s.z += Time::Delta();
        myNode->GlobalTransform().Scale() = s;
    }

    ComboEffectBase::Update();
    if (m_Expired) return;

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }
}

void ComboPetrifyConfuse::ApplyTo(EnemyBase* enemy) {
    float slowFactor = 0.5f + 0.5f * (1.0f - effect1Strength);
    float petrifyDur = duration * effect1Strength;
    enemy->ApplyPetrify(slowFactor, petrifyDur);

    float confuseDur = duration * effect2Strength;
    enemy->ApplyConfuse(confuseDur, PreciseFromCount(ingredientCount));

    spdlog::debug("ComboPetrifyConfuse: slowFactor={:.2f} petrifyDur={:.1f}s confuseDur={:.1f}s",
                  slowFactor, petrifyDur, confuseDur);
}

void ComboPetrifyConfuse::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }
}

void ComboFireConfuse::ApplyTo(EnemyBase* enemy) {
    float burnDur = duration * (1.0f - effect2Strength);
    enemy->ApplyBurn(GetDamage(), burnDur);

    float confuseDur = duration * effect2Strength;
    enemy->ApplyConfuse(confuseDur, PreciseFromCount(ingredientCount));

    enemy->TakeDamage(static_cast<int>(GetDamage()));

    spdlog::debug("ComboFireConfuse: burn={:.1f}s confuse={:.1f}s dmg={:.0f}",
                  burnDur, confuseDur, GetDamage());
}

void ComboFireConfuse::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }
}

void ComboTornadoConfuse::InitTornado() {
    m_TornadoRadius = effect2Strength * maxTornadoRadius;
    SetXZScale(myNode, m_TornadoRadius);
    spdlog::info("ComboTornadoConfuse: tornadoRadius={:.1f}", m_TornadoRadius);
}

void ComboTornadoConfuse::ApplyConfuseTo(EnemyBase* enemy) {
    float confuseDur = duration * effect2Strength;
    enemy->ApplyConfuse(confuseDur, PreciseFromCount(ingredientCount));

    spdlog::debug("ComboTornadoConfuse: confused enemy (dur={:.1f}s, precise={})",
                  confuseDur, PreciseFromCount(ingredientCount));
}

void ComboTornadoConfuse::ScanAndHandleBullets() {
    glm::vec3 pos = GetFlatPosition();

    for (auto* bullet : GetScene()->FindObjectsOfType<EnemyBullet>()) {
        if (!bullet->myNode) continue;
        glm::vec3 bpos = bullet->myNode->GlobalTransform().Position();
        if (glm::distance(pos, bpos) > m_TornadoRadius) continue;

        if (ingredientCount == 1) {
            GetScene()->QueueDelete(bullet->myNode);
        } else {
            bullet->BulletInTornadoAction(myNode, m_TornadoRadius, rotationSpeed);
        }
    }
}

void ComboTornadoConfuse::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    RotateNodeY(myNode, rotationSpeed);

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyConfuseTo(enemy);
    }

    ScanAndHandleBullets();
}