#pragma once
#include "ComboEffectBase.h"

class ComboExplodePetrify : public ComboEffectBase {
public:
    bool                    m_Initialized = false;
    ComboExplodePetrify() = default;
    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};