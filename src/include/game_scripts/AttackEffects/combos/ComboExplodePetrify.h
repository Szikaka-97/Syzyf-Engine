#pragma once
#include "ComboEffectBase.h"

/// Explode + Petrify combo effect.
/// effect1Strength → explosion damage
/// effect2Strength → petrify intensity (slow factor, duration)
///
/// The effect's XZ scale grows every frame (expanding explosion visual).
/// Enemies are hit once (hit set).
class ComboExplodePetrify : public ComboEffectBase {
public:
    void Update();

private:
    void ApplyTo(EnemyBase* enemy);
};