#pragma once
#include "ComboEffectBase.h"



class ComboFirePetrify : public ComboEffectBase {
private:

    void ApplyTo(EnemyBase* enemy);
public:

    serialized float burnDamagePerTick = 5.0f;
    serialized float burnInterval      = 1.0f;
    serialized int   debrisCount       = 7;
    serialized float debrisMinRange    = 3.0f;
    serialized float debrisMaxRange    = 15.0f;



private:
    struct FireDebris {
        SceneNode* node     = nullptr;
        glm::vec3  velocity = {};
        bool       landed   = false;
    };

    std::vector<FireDebris> m_Debris;
    std::mt19937            m_Rng{ std::random_device{}() };
public:
    ComboFirePetrify() = default;

    void Update();
    bool                    m_Initialized = false;
    void OnInit() override;

    void SpawnDebris();
    void UpdateDebris(float dt);
    void CleanupDebris();
};