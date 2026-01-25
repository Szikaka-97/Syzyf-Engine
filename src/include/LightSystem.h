#pragma once

#include <glad/glad.h>

#include <GameObjectSystem.h>
#include <Light.h>
#include <Framebuffer.h>

class LightSystem : public GameObjectSystem<Light> {
	friend class SceneGraphics;
private:
	Framebuffer* shadowAtlasFramebuffer;
	Texture2D* shadowAtlasDepthTexture;

	GLuint lightsBuffer;
	GLuint shadowmapsBuffer;

	void DoSpotLightShadowmap(Light* light, ShadowMapRegion& shadowmapRect);
	void DoDirectionalLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects);
	void DoPointLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects);
public:
	LightSystem(Scene* scene);

	GLuint GetLightsBufferHandle();
	GLuint GetShadowmapsBufferHandle();

	virtual void OnPostRender();

	virtual int Priority();
};