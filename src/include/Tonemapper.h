#pragma once

#include <PostProcessEffect.h>
#include <Debug.h>

class ComputeShaderDispatch;

class Tonemapper : public PostProcessEffect, public ImGuiDrawable {
public:
	enum class TonemapperOperator {
		None,
		Reinhard,
		Aces,
		GranTurismo,
	};
private:
	TonemapperOperator toneOperator;

	ComputeShaderDispatch* reinhardTonemapperShader;
	ComputeShaderDispatch* acesTonemapperShader;
	ComputeShaderDispatch* gtTonemapperShader;
public:
	Tonemapper();

	TonemapperOperator GetCurrentOperator() const;

	void SetOperator(TonemapperOperator opr);

	virtual void OnPostProcess(const PostProcessParams* params);

	virtual void DrawImGui();
};