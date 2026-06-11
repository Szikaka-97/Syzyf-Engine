#pragma once
#include "ComboEffectBase.h"

class ComboExplodePetrify : public ComboEffectBase {
public:
    ComboExplodePetrify() = default;
    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};