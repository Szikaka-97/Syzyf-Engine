#pragma once
#include "ComboEffectBase.h"

class ComboExplodePetrify : public ComboEffectBase {
public:
    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};