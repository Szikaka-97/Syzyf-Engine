#pragma once

#include <GameObject.h>
#include <Debug.h>
#include <TextRenderingSystem.h>

class TextRenderer : public GameObject, public ImGuiDrawable {
private:
	TextRenderingSystem::Font* font;
	std::string text;
	float size;
public:
	std::string GetText() const;
	TextRenderingSystem::Font* GetFont() const;
	float GetSize() const;

	void SetText(const std::string& newText);
	void SetFont(TextRenderingSystem::Font* newFont);
	void SetSize(float newSize);

	void DrawImGui() override;
};