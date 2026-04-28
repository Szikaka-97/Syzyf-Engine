#include "DepthOfField.h"

#include "Graphics.h"
#include "PostProcessEffect.h"
#include "Shader.h"
#include "imgui.h"

DepthOfField::DepthOfField() {
    this->savedResolution = GetScene()->GetGraphics()->GetScreenResolution();

    glCreateTextures(GL_TEXTURE_2D, 1, &this->dofTexture);

    this->UpdateTexture();
	glTextureParameteri(this->dofTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTextureParameteri(this->dofTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(this->dofTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(this->dofTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	this->downsampleShader = new ComputeShaderProgram("./res/shaders/bloom/bloom_downsample.comp");
	this->upsampleShader = new ComputeShaderProgram("./res/shaders/dof/dof_upsample.comp");
	this->finalShader = new ComputeShaderProgram("./res/shaders/dof/dof_composite.comp");
}

void DepthOfField::OnPostProcess(const PostProcessParams* params) {
	if (this->savedResolution != GetScene()->GetGraphics()->GetScreenResolution()) {
		this->savedResolution = GetScene()->GetGraphics()->GetScreenResolution();

		UpdateTexture();
	}

	glm::uvec2 resolution = glm::ceil(this->savedResolution / 2.0f);

	glUseProgram(this->downsampleShader->GetHandle());

	glBindTextureUnit(0, params->inputTexture->GetHandle());

	for (int i = 0; i < this->blurLevel - 1; i++) {
		if (i == 1) {
			glBindTextureUnit(0, this->dofTexture);
		}

		glBindImageTexture(0, this->dofTexture, i, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
		
		glm::vec2 texelSize = 1.0f / glm::vec2(resolution);
		glUniform2fv(glGetUniformLocation(this->downsampleShader->GetHandle(), "texelSize"), 1, &texelSize[0]);
		glUniform1i(glGetUniformLocation(this->downsampleShader->GetHandle(), "mipLevel"), std::max(i - 1, 0));
		glUniform1i(glGetUniformLocation(this->downsampleShader->GetHandle(), "useTreshold"), false);

		glDispatchCompute(std::ceil(float(resolution.x) / 8), std::ceil(float(resolution.y) / 8), 1);

		resolution = resolution / 2u;

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	glUseProgram(this->upsampleShader->GetHandle());

    glUniform1f(glGetUniformLocation(this->upsampleShader->GetHandle(), "bloomIntensity"), 1.0f);

	for (int i = this->blurLevel - 1; i >= 2; i--) {
		glBindImageTexture(0, this->dofTexture, i - 2, false, 0, GL_READ_WRITE, GL_RGBA16F);

		resolution.x = glm::max(1.0, glm::floor(float(savedResolution.x)  / glm::pow(2.0, i - 1)));
        resolution.y = glm::max(1.0, glm::floor(float(savedResolution.y) / glm::pow(2.0, i - 1)));

		glm::vec2 texelSize = 1.0f / glm::vec2(resolution);
		glUniform2fv(glGetUniformLocation(this->upsampleShader->GetHandle(), "texelSize"), 1, &texelSize[0]);
		glUniform1i(glGetUniformLocation(this->upsampleShader->GetHandle(), "mipLevel"), i - 1);

		glDispatchCompute(std::ceil(float(resolution.x) / 8), std::ceil(float(resolution.y) / 8), 1);

		resolution = resolution * 2u;

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

    glUseProgram(this->finalShader->GetHandle());

    glBindTextureUnit(0, params->inputTexture->GetHandle());
    glBindTextureUnit(1, this->dofTexture);
    glBindTextureUnit(2, params->depthTexture->GetHandle());
    glBindImageTexture(0, params->outputTexture->GetHandle(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

	glUniform1f(glGetUniformLocation(this->finalShader->GetHandle(), "minDistance"), this->minDistance);
	glUniform1f(glGetUniformLocation(this->finalShader->GetHandle(), "maxDistance"), this->maxDistance);
	glUniform1f(glGetUniformLocation(this->finalShader->GetHandle(), "focusDistance"), this->focusDistance);

    glUniform1i(glGetUniformLocation(this->finalShader->GetHandle(), "useDilution"), this->useDilution);
    glUniform1i(glGetUniformLocation(this->finalShader->GetHandle(), "size"), this->size);
	glUniform1f(glGetUniformLocation(this->finalShader->GetHandle(), "separation"), this->separation);
	glUniform1f(glGetUniformLocation(this->finalShader->GetHandle(), "minThreshold"), this->minThreshold);
	glUniform1f(glGetUniformLocation(this->finalShader->GetHandle(), "maxThreshold"), this->maxThreshold);
    
    glDispatchCompute(std::ceil(float(this->savedResolution.x) / 8), std::ceil(float(this->savedResolution.y) / 8), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void DepthOfField::DrawImGui() {
    ImGui::InputInt("Blur Level", &this->blurLevel);
    ImGui::InputFloat("Min Distance", &this->minDistance);
    ImGui::InputFloat("Max Distance", &this->maxDistance);
    ImGui::InputFloat("Focus Distance", &this->focusDistance);

    ImGui::Separator();
    ImGui::Checkbox("Enable Dilution", &this->useDilution);
    if (this->useDilution) {
        ImGui::InputInt("Size", &this->size);
        ImGui::InputFloat("Separation", &this->separation);
        ImGui::InputFloat("minThreshold", &this->minThreshold);
        ImGui::InputFloat("maxThreshold", &this->maxThreshold);
    }
}

void DepthOfField::UpdateTexture() {
	glm::uvec2 resolution = glm::ceil(this->savedResolution / 2.0f);

	if (resolution.x <= 0 || resolution.y <= 0) {
		return;
	}

	glBindTexture(GL_TEXTURE_2D, this->dofTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, resolution.x, resolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}
