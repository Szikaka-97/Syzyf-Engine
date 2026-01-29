#include <Tonemapper.h>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <Resources.h>
#include <Shader.h>
#include <Material.h>
#include <Graphics.h>

Tonemapper::Tonemapper() {
	this->reinhardTonemapperShader = new ComputeShaderDispatch(Resources::Get<ComputeShader>("./res/shaders/tonemapping/reinhard_tonemapper.comp"));
	this->acesTonemapperShader = new ComputeShaderDispatch(Resources::Get<ComputeShader>("./res/shaders/tonemapping/aces_tonemapper.comp"));
	this->gtTonemapperShader = new ComputeShaderDispatch(Resources::Get<ComputeShader>("./res/shaders/tonemapping/gt_tonemapper.comp"));

	this->toneOperator = TonemapperOperator::None;
}

Tonemapper::TonemapperOperator Tonemapper::GetCurrentOperator() const {
	return this->toneOperator;
}

void Tonemapper::SetOperator(Tonemapper::TonemapperOperator opr) {
	this->toneOperator = opr;
}

void Tonemapper::OnPostProcess(const PostProcessParams* params) {
	ComputeShaderDispatch* shader = nullptr;

	switch (this->toneOperator) {
		case TonemapperOperator::Reinhard:
			shader = this->reinhardTonemapperShader;
			break;
		case TonemapperOperator::Aces:
			shader = this->acesTonemapperShader;
			break;
		case TonemapperOperator::GranTurismo:
			shader = this->gtTonemapperShader;
			break;
		case TonemapperOperator::None:
		default:
			break;
	}

	if (shader) {
		shader->GetData()->SetValue("inputTex", params->inputTexture);
		shader->GetData()->SetValue("outputTex", params->outputTexture);
	
		glm::vec2 res = GetScene()->GetGraphics()->GetScreenResolution();
	
		shader->Dispatch(std::ceil(res.x / 8), std::ceil(res.y / 8), 1);
	}
}

void Tonemapper::DrawImGui() {
	const char* operators[] { "None", "Reinhard", "Aces", "Gran Turismo" };

	int currentOperator = (int) this->toneOperator;

	ImGui::Combo("Operator", &currentOperator, operators, 4);

	SetOperator((TonemapperOperator) currentOperator);
}