#pragma once
#include "ComboEffectBase.h"

class ComboPetrifyConfuse : public ComboEffectBase {
public:
    ComboPetrifyConfuse() = default;

    serialized int ingredientCount = 1;

    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};