#include "ComboEffectBase.h"
#include <spdlog/spdlog.h>

void ComboEffectBase::Init(float e1Strength, float e1MaxRange, float e1MaxDamage, float dur) {
    effect1Strength  = glm::clamp(e1Strength, 0.0f, 1.0f);
    effect2Strength  = 1.0f - effect1Strength;   
    maxEffect1Range  = e1MaxRange;
    maxEffect1Damage = e1MaxDamage;
    duration         = dur;
    m_Elapsed        = 0.0f;
    m_Expired        = false;
}

void ComboEffectBase::Update() {
    m_Elapsed += Time::Delta();
    if (m_Elapsed >= duration) {
        m_Expired = true;
        if (myNode) GetScene()->QueueDelete(myNode);
    }
}

std::vector<EnemyBase*> ComboEffectBase::ScanNearbyEnemies() const {
    float range = GetRange();
    glm::vec3 pos = GetFlatPosition();

    std::vector<EnemyBase*> result;
    for (auto* enemy : GetScene()->FindObjectsOfType<EnemyBase>()) {
        if (!enemy || !enemy->myNode) continue;
        glm::vec3 epos = enemy->myNode->GlobalTransform().Position();
        epos.y = pos.y;  
        if (glm::distance(pos, epos) <= range)
            result.push_back(enemy);
    }
    return result;
}

glm::vec3 ComboEffectBase::GetFlatPosition() const {
    glm::vec3 p = myNode ? glm::vec3(myNode->GlobalTransform().Position()) : glm::vec3(0);
    return p;
}