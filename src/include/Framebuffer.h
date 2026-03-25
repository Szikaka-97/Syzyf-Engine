#pragma once

#include <Texture.h>
#include <glm/glm.hpp>

class Framebuffer {
public:
	static constexpr int MAX_CUSTOM_ATTACHMENTS = 3;
private:
	struct FramebufferBinding {
		Texture* texture = nullptr;
		int level = 0;
		bool owning = false;
		bool enabled = false;
	};

	FramebufferBinding colorAttachment;
	FramebufferBinding depthAttachment;
	FramebufferBinding customAttachments[MAX_CUSTOM_ATTACHMENTS];

	glm::uvec2 size;

	bool dirty;
	GLuint handle;

	void SetTextureInternal(FramebufferBinding& binding, Texture* texture, int level);
public:
	enum class Attachment {
		None = 0,
		Color = 1,
		CubemappedColor = 2 | Color,
		HDRColor = 4 | Color,
		Depth = 8,
		CubemappedDepth = 8 | Depth,
		DepthStencil = 16 | Depth,
	};

	Framebuffer(Attachment attachments, unsigned int width, unsigned int height);
	Framebuffer(Attachment attachments, const glm::uvec2& size);

	Texture* GetColorTexture() const;
	int GetColorTextureLevel() const;
	Texture* GetDepthTexture() const;
	int GetDepthTextureLevel() const;
	Texture* GetCustomAttachmentTexture(int index) const;
	int GetCustomAttachmentTextureLevel(int index) const;

	GLuint GetHandle();

	Texture* CreateColorAttachment(bool hdr = false, bool cubemapped = false);
	Texture* CreateDepthAttachment(bool withStencil = false, bool cubemapped = false);
	Texture* CreateCustomAttachment(int index, const TextureParams& creationParams, bool cubemapped = false);

	void SetColorTexture(Texture* texture, int level = 0);
	void SetColorTexture(Cubemap* texture, int face);

	void SetDepthTexture(Texture* texture, int level = 0);
	void SetDepthTexture(Cubemap* texture, int face);

	void SetCustomTexture(int index, Texture* texture, int level);
	void SetCustomTexture(int index, Cubemap* texture, int face);

	bool ColorAttachmentEnabled() const;
	bool DepthAttachmentEnabled() const;
	bool CustomAttachmentEnabled(int index) const;

	void SetColorAttachmentEnabled(bool enabled);
	void SetDepthAttachmentEnabled(bool enabled);
	void SetCustomAttachmentEnabled(int index, bool enabled);

	void EnableAttachments(Attachment attachments);

	glm::uvec2 GetSize() const;
	void SetSize(const glm::uvec2& size);

	void Apply();
};

inline constexpr Framebuffer::Attachment operator&(Framebuffer::Attachment a, Framebuffer::Attachment b) {
	return static_cast<Framebuffer::Attachment>(static_cast<int>(a) & static_cast<int>(b));
}

inline constexpr Framebuffer::Attachment operator|(Framebuffer::Attachment a, Framebuffer::Attachment b) {
	return static_cast<Framebuffer::Attachment>(static_cast<int>(a) | static_cast<int>(b));
}