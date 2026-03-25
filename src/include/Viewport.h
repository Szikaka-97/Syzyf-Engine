#pragma once

#include <glm/glm.hpp>

class Framebuffer;

class Viewport {
private:
	Framebuffer* framebuffer;
public:
	Viewport();

	glm::uvec2 GetSize() const;
	Framebuffer* GetFramebuffer() const;

	void SetSize(glm::uvec2 size);
};