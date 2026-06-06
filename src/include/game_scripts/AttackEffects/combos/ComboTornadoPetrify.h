#pragma once
#include "ComboEffectBase.h"

/// Tornado + Petrify combo effect.
/// effect1Strength → (unused directly; determines effect2Strength)
/// effect2Strength → petrify intensity; also controls tornado radius
///
/// Enemies inside the radius are continuously slowed.
/// Enemy bullets are captured and orbit the tornado.
class ComboTornadoPetrify : public ComboEffectBase {
public:
    float maxTornadoRadius = 5.0f;
    float rotationSpeed    = 90.0f;

    void InitTornado();

    void Update();

private:
    float m_TornadoRadius = 0.0f;
    void ApplyPetrifyTo(EnemyBase* enemy);
};