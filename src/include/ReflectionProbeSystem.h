#pragma once

#include <GameObjectSystem.h>
#include <ReflectionProbe.h>
#include <Framebuffer.h>
#include <Texture.h>

class ReflectionProbeSystem : public GameObjectSystem<ReflectionProbe> {
private:
	Framebuffer* reflectionProbeFramebuffer;
	Texture2D* reflectionProbeDepthTexture;
public:
	ReflectionProbeSystem(Scene* scene);

	ReflectionProbe* GetClosest(glm::vec3 position);

	virtual void OnPostRender();

	virtual int Priority();
};