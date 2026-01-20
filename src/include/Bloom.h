#pragma once

#include <PostProcessEffect.h>
#include <Material.h>
#include <glad/glad.h>

class Bloom : public PostProcessEffect {
private:
	glm::vec2 savedResolution;
	GLuint bloomTexture;
	ComputeShaderProgram* downsampleShader;
	ComputeShaderProgram* upsampleShader;
	ComputeShaderProgram* finalShader;

	float threshold = 1.5f;
	float knee = 0.1f;
	float intensity = 1.0f;

	void UpdateTexture();
public:
	Bloom();

	virtual void OnPostProcess(const PostProcessParams* params);
};