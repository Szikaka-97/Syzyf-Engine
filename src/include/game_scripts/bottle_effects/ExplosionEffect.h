#pragma once

#include <game_scripts/bottle_effects/BottleEffect.h>

class ExplosionEffect : public BottleEffect {
public:
	float strength;

	virtual void Effect() override;
};