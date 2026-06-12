
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
	GLuint clustersBuffer;
	GLuint lightIndexList;
	GLuint lightIndexCounter;
	GLuint lightGrid;

	int shadowmapAtlasSize;
	int directionalLightCascadeCount;

    glm::vec4 ambientLight;

	glm::uvec3 lightGridSize;

	void ChangeShadowAtlasResolution(int newResolution);

	void DoSpotLightShadowmap(Light* light, ShadowMapRegion& shadowmapRect);
	void DoDirectionalLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects);
	void DoPointLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects);

	void RebuildLightGridBuffers();
	void CalculateLightClusters();
	void CullLights();
public:
	LightSystem(Scene* scene);

	GLuint GetLightsBufferHandle();
	GLuint GetShadowmapsBufferHandle();
	Framebuffer* GetShadowAtlasFramebuffer();
	GLuint GetLightGridHandle();
	GLuint GetLightIndexListHandle();

	glm::vec3 GetLightGridSize() const;

    glm::vec4 GetAmbientLight() const;
    void SetAmbientLight(glm::vec4 ambientLight);

	virtual void OnPostRender();

	virtual int Order();

	virtual void DrawImGui();

	virtual json Serialize();
	virtual void Deserialize(const json& data);
};

