#pragma once
#include "ComboEffectBase.h"

class ComboExplodeConfuse : public ComboEffectBase {
public:
    ComboExplodeConfuse() = default;
    serialized int ingredientCount = 1;

    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};