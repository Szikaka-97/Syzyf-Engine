#pragma once
#include "ComboEffectBase.h"

class ComboTornadoConfuse : public ComboEffectBase {
public:
    ComboTornadoConfuse() = default;

    serialized float maxTornadoRadius = 5.0f;
    serialized float rotationSpeed    = 90.0f;
    serialized int   ingredientCount  = 1;

    serialized float m_TornadoRadius = 0.0f;

    void InitTornado();

    void Update();

private:

    void ApplyConfuseTo(EnemyBase* enemy);
    void ScanAndHandleBullets();
};