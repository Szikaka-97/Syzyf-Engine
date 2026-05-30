#pragma once
#include "ComboEffectBase.h"

class ComboExplodeConfuse : public ComboEffectBase {
public:
    int ingredientCount = 1;  // 1 = random walk, 2 = attack nearest enemy

    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};