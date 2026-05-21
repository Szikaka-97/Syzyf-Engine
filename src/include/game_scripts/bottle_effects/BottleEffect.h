#pragma once

#include <GameObject.h>

class BottleEffect : public GameObject {
public:
	virtual void Effect() = 0;
};