#pragma once

#include <filesystem>
#include <unordered_set>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <VertexSpec.h>
#include <UniformSpec.h>
#include <Resources.h>

namespace fs = std::filesystem;

class ComputeShader;

template<class T>
concept ShaderLike = requires(T a) {
	{ a.GetHandle() } -> std::same_as<GLuint>;
	{ a.GetUniforms() } -> std::same_as<const UniformSpec&>;
};

const fs::path BaseShaderPath{"./res/shaders"};

class ComputeDispatchData;

struct ShaderBundle {
	GLuint vertexShaderHandle;
	GLuint geometryShaderHandle;
	GLuint tessEvalShaderHandle;
	GLuint tessCtrlShaderHandle;
	GLuint pixelShaderHandle;
};

struct ShaderKeywords {
	struct Variant {
		std::vector<std::string> keywords;
		ShaderBundle shaders;
	};

	ShaderKeywords() = default;
	
	std::map<std::string, int> keywordMap;
	std::vector<Variant> variants;
};

struct ShaderCode {
	struct KeywordInfo {
		int location;
		std::string name;
		std::string defaultValue;
	};

	std::vector<const char*> codeParts;
	std::map<std::string, KeywordInfo> keywords;
	std::vector<std::string> pragmas;
};

class Shader {
	friend class ShaderBuilder;
protected:
	ShaderCode code;
	fs::path filePath;
	bool valid;

	Shader(const fs::path& filePath, ShaderCode code);
public:
	Shader();
	static Shader LoadFromFile(const fs::path& filePath);
	
	const fs::path& GetFilePath() const;
	std::string GetName() const;
	const ShaderCode& GetCode() const;
};

class ShaderBuilder {
private:
	struct KeywordOverride {
		std::string name;
		std::string value;
	};
	fs::path vertexShaderPath;
	fs::path geometryShaderPath;
	fs::path tessEvalShaderPath;
	fs::path tessCtrlShaderPath;
	fs::path pixelShaderPath;

	GLuint CompileShader(const ShaderCode& code, std::unordered_set<std::string>& pragmas, GLenum shaderType);
	
	std::vector<KeywordOverride> keywordOverrides;
public:
	ShaderBuilder& WithVertexShader(const fs::path& vertexShaderPath);
	ShaderBuilder& WithGeometryShader(const fs::path& geometryShaderPath);
	ShaderBuilder& WithTessEvaluationShader(const fs::path& tessEvalShaderPath);
	ShaderBuilder& WithTessControlShader(const fs::path& tessCtrlShaderPath);
	ShaderBuilder& WithPixelShader(const fs::path& pixelShaderPath);

	ShaderBuilder& WithKeyword(const std::string& keyword, const std::string& keywordValue);
	ShaderBuilder& WithKeyword(const std::string& keyword, int keywordValue);
	ShaderBuilder& WithKeyword(const std::string& keyword, float keywordValue);

	ShaderProgram* Link();
};

class ShaderProgram {
	friend class ShaderBuilder;
private:
	ShaderKeywords keywords;
	ShaderKeywords::Variant* currentVariant;

	Shader vertexShader;
	Shader geometryShader;
	Shader tessEvalShader;
	Shader tessCtrlShader;
	Shader pixelShader;

	UniformSpec uniforms;
	std::unordered_set<std::string> pragmas;

	GLuint handle;

	ShaderProgram(GLuint handle);

	static std::vector<ShaderProgram*> allPrograms;
public:
	~ShaderProgram();
	static ShaderBuilder Build();
	
	GLuint GetHandle() const;
	const UniformSpec& GetUniforms() const;

	bool HasKeyword(const std::string& keyword) const;
	std::string GetKeyword(const std::string& keyword) const;
	void SetKeyword(const std::string& keyword, const std::string& keywordValue);
	void SetKeyword(const std::string& keyword, int keywordValue);
	void SetKeyword(const std::string& keyword, float keywordValue);

	bool IgnoresDepthPrepass() const;
	bool CastsShadows() const;
	bool UsesPatches() const;
	bool IsTransparent() const;

	bool HasPragma(const std::string& pragma) const;

	void Reload();
};

class ComputeShaderProgram {
private:
	ComputeShader* computeShader;
	UniformSpec uniforms;

	GLuint handle;
public:	
	ComputeShaderProgram(ComputeShader* computeShader);
	~ComputeShaderProgram();

	GLuint GetHandle() const;
	const UniformSpec& GetUniforms() const;
};

class ComputeShaderDispatch {
private:
	ComputeDispatchData* dispatchData;
	ComputeShaderProgram* program;
public:
	ComputeShaderDispatch(ComputeShader* compShader);
	ComputeShaderDispatch(ComputeShaderProgram* program);

	void Dispatch(int groupsX, int groupsY, int groupsZ) const;

	ComputeDispatchData* GetData();
	ComputeShaderProgram* GetProgram();
};
