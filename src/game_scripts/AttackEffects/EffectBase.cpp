#include "game_scripts/AttackEffects/EffectBase.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <spdlog/spdlog.h>

void EffectBase::Init() {
    m_Lifetime = 0.0f;

    OnInit();

    OnApplySpecials();

    for (auto* enemy : ScanEnemiesInRadius())
        OnApplyToEnemy(enemy);

    myNode = GetNode();

    spdlog::debug("EffectBase::Init — radius={:.1f}, maxLifetime={:.1f}s",
                  radius, GetMaxLifetime());
}

void EffectBase::Update() {
    m_Lifetime += Time::Delta() * speed;

    UpdateShaderVisual();
    OnUpdate();

    if (m_Lifetime >= GetMaxLifetime()) {
        if (myNode) GetScene()->QueueDelete(myNode);
    }
}

void EffectBase::UpdateShaderVisual() {
}

std::vector<EnemyBase*> EffectBase::ScanEnemiesInRadius() const {
    glm::vec3 pos = GetPosition();
    std::vector<EnemyBase*> result;
    for (auto* enemy : GetScene()->FindObjectsOfType<EnemyBase>()) {
        if (!enemy || !enemy->myNode) continue;
        glm::vec3 epos = enemy->myNode->GlobalTransform().Position();
        if (glm::distance(pos, epos) <= radius + 0.5f)
            result.push_back(enemy);
    }
    return result;
}

void EffectBase::SetVisual(Mesh* mesh, Material* mat) {
    SceneNode* node = GetNode();
    if (node)
        node->AddObject<MeshRenderer>(mesh, mat);
    else
        spdlog::error("EffectBase::SetVisual: GetNode() is null");
}

void EffectBase::SetEffectRenderer(Mesh* mesh, Material* mat) {
    SetVisual(mesh, mat);
}

glm::vec3 EffectBase::GetPosition() const {
    return myNode
        ? glm::vec3(myNode->GlobalTransform().Position())
        : glm::vec3(0.0f);
}