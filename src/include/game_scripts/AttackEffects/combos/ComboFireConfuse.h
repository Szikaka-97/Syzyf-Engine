#pragma once
#include "ComboEffectBase.h"

class ComboFireConfuse : public ComboEffectBase {
public:
    int ingredientCount = 1;

    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};