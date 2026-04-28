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
	serialized std::string debugName = "kurwa";

	void UpdateTexture();
public:
	Bloom();

	virtual void OnPostProcess(const PostProcessParams* params);

	virtual void DrawImGui();
};