#pragma once

#include "Framebuffer.h"
#include <glad/glad.h>
#include <GameObjectSystem.h>
#include <PostProcessEffect.h>

class PostProcessingSystem : public GameObjectSystem<PostProcessEffect> {
private:
	Framebuffer* postProcessFramebuffer;
public:
	PostProcessingSystem(Scene* scene);

	void UpdateBufferResolution(glm::vec2 newResolution);

	Framebuffer* GetPostProcessBuffer();
	void SetPostProcessBuffer(Framebuffer* val);
};