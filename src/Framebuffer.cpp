#include "Texture.h"
#include <Framebuffer.h>

void Framebuffer::SetTextureInternal(Framebuffer::FramebufferBinding& binding, Texture* texture, int level) {
	if (texture != binding.texture) {
		if (binding.texture && binding.owning) {
			delete binding.texture;
		}
	
		binding.texture = texture;

		binding.owning = false;
	}

	binding.level = level;
	binding.enabled = true;

	this->dirty = true;
}

Framebuffer::Framebuffer(Framebuffer::Attachment attachments, unsigned int width, unsigned int height):
dirty(true),
handle(0),
size(width, height) {
	bool hasColor = (attachments & Attachment::Color) == Attachment::Color;
	bool hasColorHDR = (attachments & Attachment::HDRColor) == Attachment::HDRColor;
	bool hasColorCubemap = (attachments & Attachment::CubemappedColor) == Attachment::CubemappedColor;

	bool hasDepth = (attachments & Attachment::Depth) == Attachment::Depth;
	bool hasDepthStencil = (attachments & Attachment::DepthStencil) == Attachment::DepthStencil;
	bool hasDepthCubemap = (attachments & Attachment::CubemappedDepth) == Attachment::DepthStencil;

	if (hasColor) {
		CreateColorAttachment(hasColorHDR, hasColorCubemap);
	}

	if (hasDepth) {
		CreateDepthAttachment(hasDepthStencil, hasDepthCubemap);
	}
}

Framebuffer::Framebuffer(Attachment attachments, const glm::uvec2& size):
Framebuffer(attachments, size.x, size.y) { }

Texture* Framebuffer::GetColorTexture() const {
	return this->colorAttachment.texture;
}
int Framebuffer::GetColorTextureLevel() const {
	return this->colorAttachment.level;
}
Texture* Framebuffer::GetDepthTexture() const {
	return this->depthAttachment.texture;
}
int Framebuffer::GetDepthTextureLevel() const {
	return this->depthAttachment.level;
}

Texture* Framebuffer::GetCustomAttachmentTexture(int index) const {
	if (index < 0 || index >= MAX_CUSTOM_ATTACHMENTS) {
		return nullptr;
	}

	return this->customAttachments[index].texture;
}
int Framebuffer::GetCustomAttachmentTextureLevel(int index) const {
	if (index < 0 || index >= MAX_CUSTOM_ATTACHMENTS) {
		return -1;
	}

	return this->customAttachments[index].level;
}

GLuint Framebuffer::GetHandle() {
	if (this->dirty) {
		Apply();
	}

	return this->handle;
}

Texture* Framebuffer::CreateColorAttachment(bool hdr, bool cubemapped) {
	const TextureParams& params = hdr ? Texture::HDRColorBuffer : Texture::LDRColorBuffer;

	if (this->colorAttachment.texture && this->colorAttachment.owning) {
		delete this->colorAttachment.texture;
	}

	if (cubemapped) {
		this->colorAttachment.texture = new Cubemap(this->size.x, this->size.y, params);
	}
	else {
		this->colorAttachment.texture = new Texture2D(this->size.x, this->size.y, params);
	}

	this->colorAttachment.owning = true;
	this->colorAttachment.enabled = true;
	this->dirty = true;

	return this->colorAttachment.texture;
}

Texture* Framebuffer::CreateDepthAttachment(bool withStencil, bool cubemapped) {
	const TextureParams& params = withStencil ? Texture::DepthStencilBuffer : Texture::DepthBuffer;

	if (this->depthAttachment.texture && this->depthAttachment.owning) {
		delete this->depthAttachment.texture;
	}

	if (cubemapped) {
		this->depthAttachment.texture = new Cubemap(this->size.x, this->size.y, params);
	}
	else {
		this->depthAttachment.texture = new Texture2D(this->size.x, this->size.y, params);
	}

	this->depthAttachment.owning = true;
	this->depthAttachment.enabled = true;
	this->dirty = true;

	return this->depthAttachment.texture;
}

Texture* Framebuffer::CreateCustomAttachment(int index, const TextureParams& creationParams, bool cubemapped) {
	if (index < 0 || index >= MAX_CUSTOM_ATTACHMENTS) {
		return nullptr;
	}

	if (this->customAttachments[index].texture && this->customAttachments[index].owning) {
		delete this->customAttachments[index].texture;
	}

	if (cubemapped) {
		this->customAttachments[index].texture = new Cubemap(this->size.x, this->size.y, creationParams);
	}
	else {
		this->customAttachments[index].texture = new Texture2D(this->size.x, this->size.y, creationParams);
	}

	this->customAttachments[index].owning = true;
	this->customAttachments[index].enabled = true;
	this->dirty = true;

	return this->customAttachments[index].texture;
}

void Framebuffer::SetColorTexture(Texture* texture, int level) {
	SetTextureInternal(this->colorAttachment, texture, level);
}

void Framebuffer::SetColorTexture(Cubemap* texture, int face) {
	SetColorTexture((Texture*) texture, glm::clamp(face, 0, 6));
}

void Framebuffer::SetDepthTexture(Texture* texture, int level) {
	SetTextureInternal(this->depthAttachment, texture, level);
}

void Framebuffer::SetDepthTexture(Cubemap* texture, int face) {
	SetDepthTexture((Texture*) texture, glm::clamp(face, 0, 6));
}

void Framebuffer::SetCustomTexture(int index, Texture* texture, int level) {
	if (index < 0 || index >= MAX_CUSTOM_ATTACHMENTS) {
		return;
	}

	SetTextureInternal(this->customAttachments[index], texture, level);
}

void Framebuffer::SetCustomTexture(int index, Cubemap* texture, int face) {
	SetCustomTexture(index, (Texture*) texture, glm::clamp(face, 0, 6));
}

bool Framebuffer::ColorAttachmentEnabled() const {
	return this->colorAttachment.enabled;
}
bool Framebuffer::DepthAttachmentEnabled() const {
	return this->depthAttachment.enabled;
}
bool Framebuffer::CustomAttachmentEnabled(int index) const {
	if (index < 0 || index >= MAX_CUSTOM_ATTACHMENTS) {
		return false;
	}

	return this->customAttachments[index].enabled;
}

void Framebuffer::SetColorAttachmentEnabled(bool enabled) {
	this->colorAttachment.enabled = enabled;
	this->dirty = true;
}
void Framebuffer::SetDepthAttachmentEnabled(bool enabled) {
	this->depthAttachment.enabled = enabled;
	this->dirty = true;
}
void Framebuffer::SetCustomAttachmentEnabled(int index, bool enabled) {
	if (index < 0 || index >= MAX_CUSTOM_ATTACHMENTS) {
		return;
	}

	this->customAttachments[index].enabled = enabled;
	this->dirty = true;
}

glm::uvec2 Framebuffer::GetSize() const {
	return this->size;
}

void Framebuffer::SetSize(const glm::uvec2& size) {
	this->size = size;

	if (this->colorAttachment.texture != nullptr) {
		this->colorAttachment.texture->Resize(this->size);
	}

	if (this->depthAttachment.texture != nullptr) {
		this->depthAttachment.texture->Resize(this->size);
	}

	for (int i = 0; i < MAX_CUSTOM_ATTACHMENTS; i++) {
		if (this->customAttachments[i].texture != nullptr) {
			this->customAttachments[i].texture->Resize(this->size);
		}
	}

    this->dirty = true;
}

void Framebuffer::Apply() {
	if (!this->dirty) {
		return;
	}

	if (!this->handle) {
		glCreateFramebuffers(1, &this->handle);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, this->handle);

	if (this->colorAttachment.texture && this->colorAttachment.enabled) {
		if (this->colorAttachment.texture->GetType() == TextureType::Texture2D) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment.texture->GetHandle(), this->colorAttachment.level);
		}
		else if (this->colorAttachment.texture->GetType() == TextureType::Cubemap) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + this->colorAttachment.level, this->colorAttachment.texture->GetHandle(), 0);
		}
	}
	else {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	}

	if (this->depthAttachment.texture && this->depthAttachment.enabled) {
		if (this->depthAttachment.texture->GetType() == TextureType::Texture2D) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthAttachment.texture->GetHandle(), this->depthAttachment.level);
		}
		else if (this->depthAttachment.texture->GetType() == TextureType::Cubemap) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + this->depthAttachment.level, this->depthAttachment.texture->GetHandle(), 0);
		}
	}
	else {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	}

	std::vector<GLenum> drawBuffers = {GL_COLOR_ATTACHMENT0};

	for (int i = 0; i < MAX_CUSTOM_ATTACHMENTS; i++) {
		if (this->customAttachments[i].texture && this->customAttachments[i].enabled) {
			if (this->customAttachments[i].texture->GetType() == TextureType::Texture2D) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 + i, GL_TEXTURE_2D, this->customAttachments[i].texture->GetHandle(), this->customAttachments[i].level);
			}
			else if (this->customAttachments[i].texture->GetType() == TextureType::Cubemap) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + this->customAttachments[i].level, this->customAttachments[i].texture->GetHandle(), 0);
			}

			drawBuffers.push_back(GL_COLOR_ATTACHMENT1 + i);
		}
		else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 + i, GL_TEXTURE_2D, 0, 0);
		}
	}

	glNamedFramebufferDrawBuffers(this->handle, drawBuffers.size(), drawBuffers.data());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
