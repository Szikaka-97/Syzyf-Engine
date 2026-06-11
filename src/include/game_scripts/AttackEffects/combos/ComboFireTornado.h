#pragma once
#include "ComboEffectBase.h"

class ComboFireTornado : public ComboEffectBase {
public:
    ComboFireTornado() = default;

    serialized float maxTornadoRadius = 5.0f;
    serialized float rotationSpeed    = 90.0f;
    serialized void InitTornado();

    serialized float m_TornadoRadius = 0.0f;
    void Update();

private:
    void ApplyBurnTo(EnemyBase* enemy);
};