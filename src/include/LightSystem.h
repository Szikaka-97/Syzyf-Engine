#pragma once

#include <glad/glad.h>

#include <GameObjectSystem.h>
#include <Light.h>
#include <Framebuffer.h>
#include <Debug.h>

class LightSystem : public GameObjectSystem<Light>, public ImGuiDrawable {
	friend class SceneGraphics;
private:
	Framebuffer* shadowAtlasFramebuffer;

	GLuint lightsBuffer;
	GLuint shadowmapsBuffer;

	int shadowmapAtlasSize;
	int directionalLightCascadeCount;

	void ChangeShadowAtlasResolution(int newResolution);

	void DoSpotLightShadowmap(Light* light, ShadowMapRegion& shadowmapRect);
	void DoDirectionalLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects);
	void DoPointLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects);
public:
	LightSystem(Scene* scene);

	GLuint GetLightsBufferHandle();
	GLuint GetShadowmapsBufferHandle();

	virtual void OnPostRender();

	virtual int Order();

	virtual void DrawImGui();
};