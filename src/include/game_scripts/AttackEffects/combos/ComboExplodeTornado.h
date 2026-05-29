#pragma once
#include "ComboEffectBase.h"

/// Explode + Tornado combo effect.
/// effect1Strength → explosion damage dealt to enemies inside the tornado
/// effect2Strength → tornado radius (radius = effect2Strength * maxTornadoRadius)
///
/// Enemies inside are damaged once (hit set).
/// Enemy bullets are captured and orbit.
class ComboExplodeTornado : public ComboEffectBase {
public:
    float maxTornadoRadius = 5.0f;
    float rotationSpeed    = 90.0f;

    void InitTornado();

    void Update();

private:
    float m_TornadoRadius = 0.0f;
    void ApplyTo(EnemyBase* enemy);
};