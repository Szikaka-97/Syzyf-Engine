#pragma once

#include <glad/glad.h>

#include <PostProcessEffect.h>
#include <Material.h>
#include <Debug.h>

class Bloom : public PostProcessEffect, public ImGuiDrawable {
private:
	glm::vec2 savedResolution;
	GLuint bloomTexture;
	ComputeShaderProgram* downsampleShader;
	ComputeShaderProgram* upsampleShader;
	ComputeShaderProgram* finalShader;

	serialized float threshold = 1.5f;
	serialized float knee = 0.1f;
	serialized float intensity = 0.6f;
	serialized float dirtIntensity = 0.0f;

	serialized Texture2D* dirtTexture = nullptr;

	void UpdateTexture();
public:
	Bloom();
	void Awake();

	void SetDirtTexture(Texture2D* texture);
	void SetDirtIntensity(float intensity);

	virtual void OnPostProcess(const PostProcessParams* params);

	virtual void DrawImGui();
};
