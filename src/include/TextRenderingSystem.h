#pragma once

#include "Graphics.h"
#include "Shader.h"
#include <freetype/freetype.h>

class TextRenderingSystem {
	friend class SceneGraphics;
public:
	struct Character {
		GLuint textureHandle;
		unsigned int advance;
		glm::ivec2 size;
		glm::ivec2 bearing;
	};
	struct Font {
		std::map<char, Character> characters;
		std::string name;
		FT_Face face;
	};
private:
	static FT_Library freetypeLib;
	static std::map<std::string, Font*> fonts;
	static ShaderProgram* textRenderingShader;
	static GLuint textMeshVAO;
	static GLuint textMeshVBO;
public:
	static void Reset();

	static Font* LoadFont(const std::filesystem::path& path);
};