#include "Shader.h"
#include <TextRenderingSystem.h>

#include <freetype/freetype.h>
#include <spdlog/spdlog.h>

FT_Library TextRenderingSystem::freetypeLib = 0;
std::map<std::string, TextRenderingSystem::Font*> TextRenderingSystem::fonts;
ShaderProgram* TextRenderingSystem::textRenderingShader = nullptr;
GLuint TextRenderingSystem::textMeshVAO = 0;
GLuint TextRenderingSystem::textMeshVBO = 0;

void TextRenderingSystem::Reset() {
	for (auto& f : fonts) {
		FT_Done_Face(f.second->face);

		for (auto& c : f.second->characters) {
			glDeleteTextures(1, &c.second.textureHandle);
		}
	}

	fonts.clear();

	if (freetypeLib) {
		FT_Done_FreeType(freetypeLib);
	}

	if (FT_Init_FreeType(&freetypeLib)) {
		spdlog::error("Could not load freetype library");
		
		throw 1;
	}

	if (textRenderingShader == nullptr) {
		textRenderingShader = ShaderProgram::Build()
		.WithVertexShader("./res/shaders/text/text.vert")
		.WithPixelShader("./res/shaders/text/text.frag")
		.Link();
	}

	if (textMeshVAO == 0) {
		glGenVertexArrays(1, &textMeshVAO);
		glGenBuffers(1, &textMeshVBO);
		glBindVertexArray(textMeshVAO);
		glBindBuffer(GL_ARRAY_BUFFER, textMeshVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

TextRenderingSystem::Font* TextRenderingSystem::LoadFont(const std::filesystem::path &path) {
	if (!freetypeLib) {
		return nullptr;
	}

	if (fonts.contains(path.stem().string())) {
		return fonts[path.stem().string()];
	}

	Font* newFont = new Font();

	if (FT_New_Face(freetypeLib, path.string().c_str(), 0, &newFont->face)) {
		spdlog::error("Failed to load font {}", path.string());
		
		delete newFont;

		return nullptr;
	}

	FT_Set_Pixel_Sizes(newFont->face, 0, 48);

	GLuint textureHandles[128];
	glGenTextures(128, textureHandles);

	for (unsigned char c = 0; c < 128; c++) {
		if (FT_Load_Char(newFont->face, c, FT_LOAD_RENDER)) {
			spdlog::error("Failed to load glyph {} of font {}", c, path.string());

			continue;
		}

		glBindTexture(GL_TEXTURE_2D, textureHandles[c]);

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			newFont->face->glyph->bitmap.width,
			newFont->face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			newFont->face->glyph->bitmap.buffer
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		newFont->characters[c] = {
			.textureHandle = textureHandles[c],
			.advance = (unsigned int) newFont->face->glyph->advance.x,
			.size = glm::ivec2(newFont->face->glyph->bitmap.width, newFont->face->glyph->bitmap.rows),
			.bearing= glm::ivec2(newFont->face->glyph->bitmap_left, newFont->face->glyph->bitmap_top),
		};
	}

	fonts[path.stem().string()] = newFont;

	return newFont;
}