#include "imgui.h"
#include <TextRenderer.h>
#include <cstring>

std::string TextRenderer::GetText() const {
	return this->text;
}

TextRenderingSystem::Font* TextRenderer::GetFont() const {
	return this->font;
}

float TextRenderer::GetSize() const {
	return this->size;
}

void TextRenderer::SetText(const std::string& newText) {
	this->text = newText;
}

void TextRenderer::SetFont(TextRenderingSystem::Font* newFont) {
	this->font = newFont;
}

void TextRenderer::SetSize(float newSize) {
	this->size = newSize;
}

void TextRenderer::DrawImGui() {
	char buf[256];
	strncpy(buf, this->text.c_str(), 256);

	ImGui::InputText("Text", buf, 255);

	this->text = buf;

	ImGui::InputFloat("Text size", &this->size);
}