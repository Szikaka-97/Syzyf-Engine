#include <Viewport.h>
#include <Framebuffer.h>

Viewport::Viewport():
framebuffer(new Framebuffer(Framebuffer::Attachment::None, 0, 0)) { }

glm::uvec2 Viewport::GetSize() const {
	return this->framebuffer->GetSize();
}
Framebuffer* Viewport::GetFramebuffer() const {
	return this->framebuffer;
}

void Viewport::SetSize(glm::uvec2 size) {
	this->framebuffer->SetSize(size);
}