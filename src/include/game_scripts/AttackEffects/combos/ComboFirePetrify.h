#pragma once
#include "ComboEffectBase.h"

class ComboFirePetrify : public ComboEffectBase {
private:

    void ApplyTo(EnemyBase* enemy);
public:

    ComboFirePetrify() = default;
    void Update();
};