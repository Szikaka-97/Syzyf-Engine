#pragma once

#include <filesystem>
#include <unordered_set>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <VertexSpec.h>
#include <UniformSpec.h>
#include <Resources.h>

namespace fs = std::filesystem;

template<class T>
concept ShaderLike = requires(T a) {
	{ a.GetHandle() } -> std::same_as<GLuint>;
	{ a.GetUniforms() } -> std::same_as<const UniformSpec&>;
};

const fs::path BaseShaderPath{"./res/shaders"};

class ComputeDispatchData;
class Shader;

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

class ShaderBuilder {
private:
	fs::path vertexShaderPath;
	fs::path geometryShaderPath;
	fs::path tessEvalShaderPath;
	fs::path tessCtrlShaderPath;
	fs::path pixelShaderPath;

	std::map<std::string, std::string> keywordOverrides;
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

class ComputeShaderBuilder {
private:
	struct KeywordOverride {
		std::string name;
		std::string value;
	};

	fs::path shaderPath;

	std::vector<KeywordOverride> keywordOverrides;
public:
	ComputeShaderBuilder& WithComputeShader(const fs::path& shaderPath);

	ComputeShaderBuilder& WithKeyword(const std::string& keyword, const std::string& keywordValue);
	ComputeShaderBuilder& WithKeyword(const std::string& keyword, int keywordValue);
	ComputeShaderBuilder& WithKeyword(const std::string& keyword, float keywordValue);

	ComputeShaderProgram* Link();
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
	
	static ShaderBuilder Build();
	static ComputeShaderBuilder BuildCompute();
	static ComputeShaderBuilder BuildCompute(const fs::path& shaderPath);

	const fs::path& GetFilePath() const;
	std::string GetName() const;
	const ShaderCode& GetCode() const;
};

class ShaderProgram : Resource {
	friend class ShaderBuilder;
private:
	struct ShaderAttachment {
		Shader shader;
		GLuint handle = 0;

		inline bool Attached() const {
			return handle != 0;
		}
	};

	std::map<std::string, std::string> keywords;

	ShaderAttachment vertexShader;
	ShaderAttachment geometryShader;
	ShaderAttachment tessEvalShader;
	ShaderAttachment tessCtrlShader;
	ShaderAttachment pixelShader;

	UniformSpec uniforms;
	std::unordered_set<std::string> pragmas;

	GLuint handle;

	ShaderProgram(GLuint handle);

	fs::path path;

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

	const Shader GetVertexShader() const;
	const Shader GetGeometryShader() const;
	const Shader GetTessCtrlShader() const;
	const Shader GetTessEvalShader() const;
	const Shader GetPixelShader() const;

	bool HasPragma(const std::string& pragma) const;

	virtual fs::path GetName() const;

	void Reload();

	static void ReloadAllShaders();

	static ShaderProgram* Load(const fs::path& shaderPath);
};

class ComputeShaderProgram {
	friend class ComputeShaderBuilder;
private:
	Shader computeShader;
	UniformSpec uniforms;

	GLuint handle;
	ComputeShaderProgram(GLuint handle);
public:	
	ComputeShaderProgram(const fs::path& shaderPath);
	~ComputeShaderProgram();

	static ComputeShaderBuilder Build();
	static ComputeShaderBuilder Build(const fs::path& shaderPath);

	GLuint GetHandle() const;
	const UniformSpec& GetUniforms() const;
};

class ComputeShaderDispatch {
private:
	ComputeDispatchData* dispatchData;
	ComputeShaderProgram* program;
public:
	ComputeShaderDispatch(const fs::path& shaderPath);
	ComputeShaderDispatch(ComputeShaderProgram* program);

	void Dispatch(int groupsX, int groupsY, int groupsZ) const;

	ComputeDispatchData* GetData();
	ComputeShaderProgram* GetProgram();
};
