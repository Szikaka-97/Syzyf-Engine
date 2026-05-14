#include "ComboFirePetrify.h"
#include "ComboFireTornado.h"
#include "ComboTornadoPetrify.h"
#include "ComboExplodePetrify.h"
#include "ComboExplodeTornado.h"
#include <TimeSystem.h>
#include <spdlog/spdlog.h>

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