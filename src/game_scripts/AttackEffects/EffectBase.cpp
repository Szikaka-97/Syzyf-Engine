#include "EffectBase.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <spdlog/spdlog.h>



void EffectBase::Init() {
    OnApplySpecials();   

    for (auto* enemy : ScanEnemiesInRadius())
        OnApplyToEnemy(enemy);

    spdlog::debug("PotionEffectBase: Init complete, radius={:.1f}, maxLifetime={:.1f}",
                  radius, GetMaxLifetime());

    myNode = GetNode();
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
    if (!m_EffectRenderer) return;
    float t      = (radius > 0.0f) ? (m_Lifetime / radius) : 0.0f;
    float factor = std::sin(t * glm::half_pi<float>());
   // m_EffectMaterial->SetValue("_ExplosionTime", factor * radius);
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

void EffectBase::SetEffectRenderer(Mesh* mesh, Material* mat) {
    SceneNode* node = GetNode();
    if (node) {
        node->AddObject<MeshRenderer>(mesh, mat);
    } else {
        spdlog::error("EffectBase::SetEffectRenderer: GetNode() is null – effect not attached to any node");
    }
}

glm::vec3 EffectBase::GetPosition() const {
    return myNode ? glm::vec3(myNode->GlobalTransform().Position()) : glm::vec3(0.0f);
}