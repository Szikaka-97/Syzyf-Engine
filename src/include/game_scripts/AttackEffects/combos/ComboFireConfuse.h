#pragma once
#include "ComboEffectBase.h"

class ComboFireConfuse : public ComboEffectBase {
public:
    ComboFireConfuse() = default;
    serialized int ingredientCount = 1;

    serialized float burnDamagePerTick = 5.0f;
    serialized float burnInterval      = 1.0f;
    serialized int   debrisCount       = 7;
    serialized float debrisMinRange    = 3.0f;
    serialized float debrisMaxRange    = 15.0f;

    void Update();

private:
    struct FireDebris {
        SceneNode* node     = nullptr;
        glm::vec3  velocity = {};
        bool       landed   = false;
    };

    bool                    m_Initialized = false;
    std::vector<FireDebris> m_Debris;
    std::mt19937            m_Rng{ std::random_device{}() };
    void ApplyTo(EnemyBase* enemy);
    void SpawnDebris();
    void UpdateDebris(float dt);
    void CleanupDebris();

};