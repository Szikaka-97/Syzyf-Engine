#include <Framebuffer.h>

Framebuffer::Framebuffer(Texture2D* colorTexture, int colorTextureLevel, Texture2D* depthTexture, int depthTextureLevel):
colorTexture(colorTexture),
colorTextureLevel(colorTextureLevel),
depthTexture(depthTexture),
depthTextureLevel(depthTextureLevel),
dirty(true),
handle(0) {
	Apply();
}

Framebuffer::Framebuffer(Texture2D* colorTexture, int colorTextureLevel, Texture2D* depthTexture):
Framebuffer(colorTexture, colorTextureLevel, depthTexture, 0) { }

Framebuffer::Framebuffer(Texture2D* colorTexture, int colorTextureLevel):
Framebuffer(colorTexture, colorTextureLevel, nullptr, 0) { }

Framebuffer::Framebuffer(Texture2D* colorTexture, Texture2D* depthTexture):
Framebuffer(colorTexture, 0, depthTexture, 0) { }

Framebuffer::Framebuffer(Cubemap* colorTexture, int face, Texture2D* depthTexture, int depthTextureLevel):
colorTexture(colorTexture),
colorTextureLevel(face),
depthTexture(depthTexture),
depthTextureLevel(depthTextureLevel),
dirty(true),
handle(0) {
	Apply();
}

Framebuffer::Framebuffer(Cubemap* colorTexture, int face, Texture2D* depthTexture):
Framebuffer(colorTexture, face, depthTexture, 0) { }

Framebuffer::Framebuffer(Cubemap* colorTexture, int face):
Framebuffer(colorTexture, face, nullptr, 0) { }

Texture* Framebuffer::GetColorTexture() const {
	return this->colorTexture;
}
int Framebuffer::GetColorTextureLevel() const {
	return this->colorTextureLevel;
}
Texture* Framebuffer::GetDepthTexture() const {
	return this->depthTexture;
}
int Framebuffer::GetDepthTextureLevel() const {
	return this->depthTextureLevel;
}

GLuint Framebuffer::GetHandle() {
	if (this->dirty) {
		Apply();
	}

	return this->handle;
}

void Framebuffer::SetColorTexture(Texture2D* texture, int level) {
	this->colorTexture = texture;
	this->colorTextureLevel = level;

	this->dirty = true;
}
void Framebuffer::SetColorTexture(Cubemap* texture, int face) {
	this->colorTexture = texture;
	this->colorTextureLevel = face;

	this->dirty = true;
}
void Framebuffer::SetDepthTexture(Texture* texture, int level) {
this->colorTexture = texture;
	this->depthTexture = texture;
	this->depthTextureLevel = level;

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

	if (this->colorTexture) {
		if (this->colorTexture->GetType() == TextureType::Texture2D) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->colorTexture->GetHandle(), this->colorTextureLevel);
		}
		else if (this->colorTexture->GetType() == TextureType::Cubemap) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + this->colorTextureLevel, this->colorTexture->GetHandle(), 0);
		}
	}

	if (this->depthTexture) {
		if (this->depthTexture->GetType() == TextureType::Texture2D) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthTexture->GetHandle(), this->depthTextureLevel);
		}
		else if (this->depthTexture->GetType() == TextureType::Cubemap) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + this->depthTextureLevel, this->depthTexture->GetHandle(), 0);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}