#pragma once
#include "ComboEffectBase.h"

class ComboExplodeTornado : public ComboEffectBase {
public:
    ComboExplodeTornado() = default;
    serialized float maxTornadoRadius = 5.0f;
    serialized float rotationSpeed    = 90.0f;

    serialized float m_TornadoRadius = 0.0f;
    void InitTornado();

    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};