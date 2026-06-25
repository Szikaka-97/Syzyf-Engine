#include "./include/game_scripts/AttackEffects/combos/ComboExplodeFire.h"
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

void ComboExplodeFire::OnInit() {
    SceneNode* explosionModel =
           ResourceDatabase::Global
   ->Get<GltfScene>("./res/models/effects/explode.glb")
->Instantiate(GetScene(), GetNode(), "explosion effect");
    explosionModel->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    explosionModel->LocalTransform().Position() = GetFlatPosition();
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    //this->radius = GetRange();

    for (auto& anim : explosionModel->GetObject<AnimationComponent>()->animations) {
        anim.speed = 2.0;
        explosionModel->GetObject<AnimationComponent>()->Play(anim.data.name);
    }

    //explosionModel->FindNode("SphereScaler")->LocalTransform().Scale() = glm::vec3(this->radius);
}

void ComboExplodeFire::ApplyTo(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));

    float actualBurnDmg = burnDamagePerTick * effect2Strength;
    float burnDuration  = duration * effect2Strength;
    enemy->ApplyBurn(actualBurnDmg, burnDuration, burnInterval);

    spdlog::debug("ComboExplodeFire: hit enemy — dmg={:.0f}, burn={:.1f}/s for {:.1f}s",
                  GetDamage(), actualBurnDmg, burnDuration);
}

void ComboExplodeFire::SpawnDebris() {
    static const float g = 9.81f;

    const int count = std::clamp(debrisCount, 5, 10);
    m_Debris.reserve(count);

    std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> rangeDist(debrisMinRange, debrisMaxRange);

    glm::vec3 origin = GetFlatPosition();

    for (int i = 0; i < count; ++i) {
        float angle = angleDist(m_Rng);
        float range = rangeDist(m_Rng);
        float v     = std::sqrt(range * g);

        glm::vec3 horiz(std::cos(angle), 0.0f, std::sin(angle));

        constexpr float cos45 = 0.7071068f;
        glm::vec3 vel = horiz * (v * cos45) + glm::vec3(0.0f, v * cos45, 0.0f);

        SceneNode* fireNode = GetScene()->CreateNode("Fire");
        fireNode->GlobalTransform().Position() = origin;
        fireNode->AddObject<FireParticles>();

        m_Debris.push_back({ fireNode, vel, false });
    }

    spdlog::info("ComboExplodeFire: spawned {} fire debris (range {:.1f}–{:.1f})",
                 count, debrisMinRange, debrisMaxRange);
}

void ComboExplodeFire::UpdateDebris(float dt) {
    static const float g = 9.81f;
    float groundY = GetFlatPosition().y;

    for (auto& d : m_Debris) {
        if (d.landed || !d.node) continue;

        d.velocity.y -= g * dt;
        glm::vec3 pos  = d.node->GlobalTransform().Position();
        pos            += d.velocity * dt;

        if (pos.y <= groundY) {
            pos.y      = groundY;
            d.velocity = glm::vec3(0.0f);
            d.landed   = true;
        }

        d.node->GlobalTransform().Position() = pos;
    }
}

void ComboExplodeFire::CleanupDebris() {
    for (auto& d : m_Debris)
        if (d.node) GetScene()->QueueDelete(d.node);
    m_Debris.clear();
}

void ComboExplodeFire::Update() {
    ComboEffectBase::Update();
    if (m_Expired) return;

    if (!m_Initialized) {
        SceneNode* explosionModel = ResourceDatabase::Global
            ->Get<GltfScene>("./res/models/effects/explode.glb")
            ->Instantiate(GetScene(), GetNode(), "explosion effect");
        explosionModel->GlobalTransform().Position() = GetFlatPosition();
        if (explosionModel) {
            explosionModel->GlobalTransform().Scale() = glm::vec3(1.0f);

            if (auto* anim = explosionModel->GetObject<AnimationComponent>()) {
                for (auto& a : anim->animations) {
                    a.speed = 2.0f;
                    anim->Play(a.data.name);
                }
            }
        }

        if (GetNode())
            GetNode()->GlobalTransform().Scale() = glm::vec3(1.0f);

        SpawnDebris();
        m_Initialized = true;
    }

    UpdateDebris(Time::Delta());

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }
}

void ComboFirePetrify::SpawnDebris() {
    static const float g = 9.81f;

    const int count = std::clamp(debrisCount, 5, 10);
    m_Debris.reserve(count);

    std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> rangeDist(debrisMinRange, debrisMaxRange);

    glm::vec3 origin = GetFlatPosition();

    for (int i = 0; i < count; ++i) {
        float angle = angleDist(m_Rng);
        float range = rangeDist(m_Rng);
        float v     = std::sqrt(range * g);

        glm::vec3 horiz(std::cos(angle), 0.0f, std::sin(angle));

        constexpr float cos45 = 0.7071068f;
        glm::vec3 vel = horiz * (v * cos45) + glm::vec3(0.0f, v * cos45, 0.0f);

        SceneNode* fireNode = GetScene()->CreateNode("Fire");
        fireNode->GlobalTransform().Position() = origin;
        fireNode->AddObject<FireParticles>();

        m_Debris.push_back({ fireNode, vel, false });
    }

    spdlog::info("ComboFirePetrify: spawned {} fire debris (range {:.1f}–{:.1f})",
                 count, debrisMinRange, debrisMaxRange);
}

void ComboFirePetrify::UpdateDebris(float dt) {
    static const float g = 9.81f;
    float groundY = GetFlatPosition().y;

    for (auto& d : m_Debris) {
        if (d.landed || !d.node) continue;

        d.velocity.y -= g * dt;
        glm::vec3 pos  = d.node->GlobalTransform().Position();
        pos            += d.velocity * dt;

        if (pos.y <= groundY) {
            pos.y      = groundY;
            d.velocity = glm::vec3(0.0f);
            d.landed   = true;
        }

        d.node->GlobalTransform().Position() = pos;
    }
}

void ComboFirePetrify::CleanupDebris() {
    for (auto& d : m_Debris)
        if (d.node) GetScene()->QueueDelete(d.node);
    m_Debris.clear();
}

void ComboFirePetrify::OnInit() {

}

void ComboFirePetrify::ApplyTo(EnemyBase* enemy) {
    float burnDuration = duration * (1.0f - effect2Strength);
    enemy->ApplyBurn(GetDamage(), burnDuration);
    enemy->ApplyConfuse(duration,false);

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
    if (!m_Initialized) {
        SceneNode* explosionModel = ResourceDatabase::Global
            ->Get<GltfScene>("./res/models/effects/petrify.glb")
            ->Instantiate(GetScene(), GetNode(), "ComboFirePetrify effect");
        //explosionModel->GlobalTransform().Position() = GetFlatPosition();
        explosionModel->GlobalTransform().Position() = glm::vec3(GetFlatPosition().x ,0.f, GetFlatPosition().z);
        if (explosionModel) {
            explosionModel->GlobalTransform().Scale() = glm::vec3(1.0f);

            if (auto* anim = explosionModel->GetObject<AnimationComponent>()) {
                for (auto& a : anim->animations) {
                    a.speed = 2.0f;
                    anim->Play(a.data.name);
                }
            }
        }

        if (GetNode())
            GetNode()->GlobalTransform().Scale() = glm::vec3(1.0f);

        SpawnDebris();
        m_Initialized = true;
    }

}

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

}

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
void ComboExplodeConfuse::OnInit() {
    SceneNode* explosionModel =
          ResourceDatabase::Global
  ->Get<GltfScene>("./res/models/effects/combos/confuse_explode.glb")
->Instantiate(GetScene(), GetNode(), "ComboExplodeConfuse effect");
    explosionModel->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    explosionModel->LocalTransform().Position() = glm::vec3(0.0f);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    //this->radius = GetRange();

    for (auto& anim : explosionModel->GetObject<AnimationComponent>()->animations) {
        anim.speed = 2.0;
        explosionModel->GetObject<AnimationComponent>()->Play(anim.data.name);
    }
}

static bool PreciseFromCount(int count) { return count == 2; }

void ComboExplodeConfuse::ApplyTo(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));

    float confuseDur = duration * effect2Strength;
    enemy->ApplyConfuse(confuseDur, PreciseFromCount(ingredientCount));

    spdlog::debug("ComboExplodeConfuse: hit enemy � dmg={:.0f}, confuseDur={:.1f}s",
                  GetDamage(), confuseDur);
}

void ComboExplodeConfuse::Update() {
    // if (myNode) {
    //     glm::vec3 s = myNode->GlobalTransform().Scale();
    //     s.x += Time::Delta();
    //     s.z += Time::Delta();
    //     myNode->GlobalTransform().Scale() = s;
    // }

    ComboEffectBase::Update();
    if (m_Expired) return;

    for (auto* enemy : ScanNearbyEnemies()) {
        if (m_HitEnemies.count(enemy)) continue;
        m_HitEnemies.insert(enemy);
        ApplyTo(enemy);
    }

    if (!m_Initialized) {
        SceneNode* explosionModel =
          ResourceDatabase::Global
  ->Get<GltfScene>("./res/models/effects/combos/confuse_explode.glb")
->Instantiate(GetScene(), GetNode(), "ComboExplodeConfuse effect");
        explosionModel->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
        explosionModel->LocalTransform().Position() = glm::vec3(0.0f);
        GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
        //this->radius = GetRange();

        for (auto& anim : explosionModel->GetObject<AnimationComponent>()->animations) {
            anim.speed = 2.0;
            explosionModel->GetObject<AnimationComponent>()->Play(anim.data.name);
        }

        if (GetNode())
            GetNode()->GlobalTransform().Scale() = glm::vec3(1.0f);

        //SpawnDebris();
        m_Initialized = true;
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