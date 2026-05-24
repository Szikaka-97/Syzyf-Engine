#pragma once
#include "ComboEffectBase.h"

class ComboTornadoConfuse : public ComboEffectBase {
public:
    float maxTornadoRadius = 5.0f;
    float rotationSpeed    = 90.0f;  // degrees/s
    int   ingredientCount  = 1;

    /// Wywo³aj po Init() — ustawia skalê wêz³a na podstawie effect2Strength.
    void InitTornado();

    void Update();

private:
    float m_TornadoRadius = 0.0f;

    void ApplyConfuseTo(EnemyBase* enemy);
    void ScanAndHandleBullets();
};