#pragma once

#include <Texture.h>

class Framebuffer {
private:
	Texture* colorTexture;
	int colorTextureLevel;

	Texture* depthTexture;
	int depthTextureLevel;

	bool dirty;
	GLuint handle;
public:
	Framebuffer(Texture2D* colorTexture, int colorTextureLevel, Texture2D* depthTexture, int depthTextureLevel);
	Framebuffer(Texture2D* colorTexture, int colorTextureLevel, Texture2D* depthTexture);
	Framebuffer(Texture2D* colorTexture, int colorTextureLevel);
	Framebuffer(Texture2D* colorTexture, Texture2D* depthTexture);

	Framebuffer(Cubemap* colorTexture, int face, Texture2D* depthTexture, int depthTextureLevel);
	Framebuffer(Cubemap* colorTexture, int face, Texture2D* depthTexture);
	Framebuffer(Cubemap* colorTexture, int face);

	Texture* GetColorTexture() const;
	int GetColorTextureLevel() const;
	Texture* GetDepthTexture() const;
	int GetDepthTextureLevel() const;

	GLuint GetHandle();

	void SetColorTexture(Texture2D* texture, int level = 0);
	void SetColorTexture(Cubemap* texture, int face);
	void SetDepthTexture(Texture* texture, int level);

	void Apply();
};