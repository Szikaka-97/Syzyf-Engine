#pragma once

#include <GameObjectSystem.h>
#include <ReflectionProbe.h>
#include <Framebuffer.h>
#include <Texture.h>
#include <Debug.h>

class ReflectionProbeSystem : public GameObjectSystem<ReflectionProbe>, public ImGuiDrawable {
private:
	ReflectionProbe* skyboxProbe;

	Framebuffer* reflectionProbeFramebuffer;
	Cubemap* reflectionProbeColorTexture;
	Texture2D* reflectionProbeDepthTexture;

	Texture2D* brdfConvolutionMap;
public:
	ReflectionProbeSystem(Scene* scene);

	void RecalculateSkyboxIBL();

	void InvalidateAll();

	ReflectionProbe* GetClosestProbe(glm::vec3 position);

	Texture2D* BRDFConvolutionMap();

	virtual void OnPostRender();

	virtual int Order();

	virtual void DrawImGui();
};