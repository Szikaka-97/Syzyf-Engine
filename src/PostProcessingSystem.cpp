#include "Framebuffer.h"
#include <PostProcessingSystem.h>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>

PostProcessingSystem::PostProcessingSystem(Scene* scene):
GameObjectSystem<PostProcessEffect>(scene),
postProcessFramebuffer() {
	this->postProcessFramebuffer = new Framebuffer(Framebuffer::Attachment::HDRColor | Framebuffer::Attachment::Depth, glm::zero<glm::uvec2>());
}

void PostProcessingSystem::UpdateBufferResolution(glm::vec2 newResolution) {
	this->postProcessFramebuffer->SetSize(newResolution);
}

Framebuffer* PostProcessingSystem::GetPostProcessBuffer() {
	return this->postProcessFramebuffer;
}

void PostProcessingSystem::SetPostProcessBuffer(Framebuffer* val) {
	this->postProcessFramebuffer = val;
}