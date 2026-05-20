#pragma once
#include "ComboEffectBase.h"

/// Fire + Tornado combo effect.
/// effect1Strength → fire intensity (DOT damage)
/// effect2Strength → tornado size (radius = effect2Strength * maxTornadoRadius)
///
/// Enemies entering the radius are burned.
/// Enemy bullets that come close are captured and orbit the tornado.
/// The effect node visually rotates each frame.
class ComboFireTornado : public ComboEffectBase {
public:
    float maxTornadoRadius = 5.0f;
    float rotationSpeed    = 90.0f;  // degrees/s

    /// Call after Init() to set tornado-specific radius on the scene node scale.
    void InitTornado();

    void Update();

private:
    float m_TornadoRadius = 0.0f;
    void ApplyBurnTo(EnemyBase* enemy);
};