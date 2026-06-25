#pragma once
#include "ComboEffectBase.h"

class ComboExplodeConfuse : public ComboEffectBase {
public:
    ComboExplodeConfuse() = default;
    serialized int ingredientCount = 1;
    bool                    m_Initialized = false;

    void Update();
    void OnInit() override;

private:
    void ApplyTo(EnemyBase* enemy);
};