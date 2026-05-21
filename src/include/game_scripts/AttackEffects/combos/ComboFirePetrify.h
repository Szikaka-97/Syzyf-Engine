#pragma once
#include "ComboEffectBase.h"

/// Fire + Petrify combo effect.
/// effect1Strength → fire intensity (DOT damage, duration)
/// effect2Strength → petrify intensity (slow factor, slow duration)
///
/// One-shot per enemy: each enemy is burned + slowed once on first contact.
class ComboFirePetrify : public ComboEffectBase {
private:
    void ApplyTo(EnemyBase* enemy);
public:
    void Update();
};