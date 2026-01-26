#pragma once

#include <GameObject.h>

#include <Texture.h>

class ReflectionProbeSystem;

class ReflectionProbe : public GameObject {
	friend class ReflectionProbeSystem;
private:
	static constexpr unsigned int resolution = 128;

	bool dirty;
	Cubemap* cubemap;
public:
	ReflectionProbe();

	void Regenerate();

	Cubemap* GetCubemapTexture();

	void Render();
};