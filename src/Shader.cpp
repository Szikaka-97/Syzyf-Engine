#include "Texture.h"
#include "UniformSpec.h"
#include <Shader.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <malloc.h>
#include <stb_include.h>

#include <PreComp.h>
#include <Material.h>

#include <spdlog/spdlog.h>
#include <string_view>

constexpr std::string VertexShaderExtension = ".vert";
constexpr std::string GeometryShaderExtension = ".geom";
constexpr std::string TesselationControlShaderExtension = ".tess_eval";
constexpr std::string TesselationEvaluationShaderExtension = ".tess_ctrl";
constexpr std::string PixelShaderExtension = ".frag";
constexpr std::string ComputeShaderExtension = ".comp";

std::vector<ShaderProgram*> ShaderProgram::allPrograms;

Shader::Shader():
valid(false) { }

Shader::Shader(const fs::path& filePath, ShaderCode code):
filePath(filePath),
code(code),
valid(true) { }

char* LoadFile(const fs::path& filePath) {
	fs::directory_entry shaderFile(filePath);

	char* buf = new char[shaderFile.file_size() + 1];
	
	std::ifstream shaderFileStream(filePath, std::ios::binary);

	shaderFileStream.read(buf, shaderFile.file_size());
	
	buf[shaderFile.file_size()] = '\0';
	
	return buf;
}

VertexSpec GetVertexSpec(const std::string_view& shaderCode) {
	auto inputSearch = std::regex_iterator(shaderCode.cbegin(), shaderCode.cend(), Regex::shaderInputRegex);
	decltype(inputSearch) shaderCodeEnd;
	std::vector<VertexInput> spec;

	for (auto i = inputSearch; i != shaderCodeEnd; ++i) {
		const auto match = *i;

		const std::string semanticString = match[2].str();

		const VertexInputType semantic = VertexSpec::TypeFromSemantic(semanticString);

		if ((int) semantic < 0) {
			spdlog::error("Unknown shader input semantic: " + semanticString);
		}

		const std::string typeStr = match[3].str();
		int length = 0;

		length = typeStr.back() - '0';

		if (length < 0) {
			length = 1;
		}

		spec.push_back(VertexInput(semantic, length));
	}

	return VertexSpec(spec);
}

Shader Shader::LoadFromFile(const fs::path& filePath) {
	char preprocessorErrorMsg[256];

	char* shaderSource = LoadFile(filePath);

	char* preprocessedSource = stb_include_string(shaderSource, (char* ) "", (char* ) "./res/shaders", filePath.string().data(), preprocessorErrorMsg);

	if (preprocessedSource == nullptr) {
		spdlog::error("Error preprocessing shader {}:\n{}", filePath.string(), std::string(preprocessorErrorMsg));

		throw "Preprocessor exception";
	}

	ShaderCode code;
	code.codeParts.push_back("#version 460\n");

#ifdef __linux__
	code.codeParts.push_back("#define _LINUX\n");
#elif _WIN32
	code.codeParts.push_back("#define _WIN32\n");
#else
	code.codeParts.push_back("#define OS_OTHER\n");
#endif

	code.codeParts.push_back(preprocessedSource);
	int partLength = 0;
	int partNum = 2;

	std::stringstream shaderLines(preprocessedSource);

	for (std::string line; std::getline(shaderLines, line); ) {
		auto partStr = code.codeParts.back();
		
		std::smatch variantMatch{};
		if (std::regex_search(line, variantMatch, Regex::shaderVersionRegex)) {
			*const_cast<char*>(partStr + partLength) = '\0';

			code.codeParts.push_back(partStr + partLength + line.length() + 1);

			partLength = 0;

			partNum++;
		}
		else if (std::regex_match(line, variantMatch, Regex::shaderPragmaRegex)) {
			*const_cast<char*>(partStr + partLength) = '\0';

			code.pragmas.push_back(variantMatch[1].str());

			code.codeParts.push_back(partStr + partLength + line.length() + 1);

			partLength = 0;

			partNum++;
		}
		else if (std::regex_match(line, variantMatch, Regex::shaderVariantRegex)) {
			int keywordNameStartOffset = variantMatch[1].first - variantMatch[0].first;
			int keywordNameEndOffset = variantMatch[1].second - variantMatch[0].first;

			int keywordValueStartOffset = variantMatch[3].first - variantMatch[0].first;
			int keywordValueEndOffset = variantMatch[3].second - variantMatch[0].first;

			*const_cast<char*>(partStr + partLength) = '\0';
			*const_cast<char*>(partStr + partLength + keywordNameEndOffset) = '\0';
			*const_cast<char*>(partStr + partLength + keywordValueEndOffset) = '\0';

			spdlog::warn(variantMatch[3].str());
			spdlog::warn(keywordValueStartOffset);
			spdlog::warn(keywordValueEndOffset);

			code.codeParts.push_back("\n#define ");
			code.codeParts.push_back(partStr + partLength + keywordNameStartOffset);
			code.codeParts.push_back(" ");
			code.codeParts.push_back(partStr + partLength + keywordValueStartOffset);
			code.codeParts.push_back(partStr + partLength + line.length() + 1);
			partLength = 0;

			std::string keywordName = variantMatch[1].str();
			std::string keywordValue = variantMatch[3].str();

			if (code.keywords.contains(keywordName)) {
				throw "A keyword definition can only appear once in a shader file";
			}

			code.keywords[keywordName] = {
				partNum + 4,
				keywordName,
				keywordValue
			};

			partNum += 5;
		}
		else {
			partLength += line.length() + 1;
		}
	}

	Shader result(filePath, code);

	return result;
}

ShaderBuilder Shader::Build() {
	return ShaderBuilder();
}
ComputeShaderBuilder Shader::BuildCompute() {
	return ComputeShaderBuilder();
}
ComputeShaderBuilder Shader::BuildCompute(const fs::path& shaderPath) {
	auto result = ComputeShaderBuilder();

	result.WithComputeShader(shaderPath);

	return result;
}

const fs::path& Shader::GetFilePath() const {
	return this->filePath;
}

std::string Shader::GetName() const {
	return this->filePath.stem().string();
}

const ShaderCode& Shader::GetCode() const {
	return this->code;
}

ShaderBuilder& ShaderBuilder::WithVertexShader(const fs::path& vertexShaderPath) {
	this->vertexShaderPath = vertexShaderPath;

	return *this;
}
ShaderBuilder& ShaderBuilder::WithGeometryShader(const fs::path& geometryShaderPath) {
	this->geometryShaderPath = geometryShaderPath;

	return *this;
}
ShaderBuilder& ShaderBuilder::WithTessEvaluationShader(const fs::path& tessEvalShaderPath) {
	this->tessEvalShaderPath = tessEvalShaderPath;

	return *this;
}
ShaderBuilder& ShaderBuilder::WithTessControlShader(const fs::path& tessCtrlShaderPath) {
	this->tessCtrlShaderPath = tessCtrlShaderPath;

	return *this;
}
ShaderBuilder& ShaderBuilder::WithPixelShader(const fs::path& pixelShaderPath) {
	this->pixelShaderPath = pixelShaderPath;

	return *this;
}

ShaderBuilder& ShaderBuilder::WithKeyword(const std::string& keyword, const std::string& keywordValue) {
	this->keywordOverrides[keyword] = keywordValue;

	return *this;
}
ShaderBuilder& ShaderBuilder::WithKeyword(const std::string& keyword, int keywordValue) {
	this->keywordOverrides[keyword] = std::to_string(keywordValue);

	return *this;
}
ShaderBuilder& ShaderBuilder::WithKeyword(const std::string& keyword, float keywordValue) {
	this->keywordOverrides[keyword] = std::to_string(keywordValue);

	return *this;
}

GLuint CompileShader(const Shader& shader, std::map<std::string, std::string>& keywords, GLenum shaderType) {
	const ShaderCode& code = shader.GetCode();

	std::vector<const char*> finalParts(code.codeParts.size());

	for (int i = 0; i < code.codeParts.size(); i++) {
		finalParts[i] = code.codeParts[i];
	}

	for (const auto& keyword : keywords) {
		auto keywordIt = code.keywords.find(keyword.first);

		if (keywordIt != code.keywords.end()) {
			finalParts[keywordIt->second.location] = keyword.second.c_str();
		}
	}

	GLuint handle = glCreateShader(shaderType);

	glShaderSource(handle, finalParts.size(), finalParts.data(), nullptr);

	glCompileShader(handle);

	GLint compileSuccess;

	glGetShaderiv(handle, GL_COMPILE_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;

		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetShaderInfoLog(handle, messageLength, &messageLength, infoLog);

		spdlog::error("Error compiling shader {}", fs::canonical(shader.GetFilePath()).string().c_str());

		std::istringstream shaderLines(infoLog);

		for (std::string line; std::getline(shaderLines, line); ) {
			spdlog::error(std::string(line));
		}

		throw 1;
	}

	return handle;
}

ShaderProgram* ShaderBuilder::Link() {
	Shader vertexShader,
	       geometryShader,
	       tessEvalShader,
	       tessCtrlShader,
	       pixelShader;

	GLuint vertexShaderHandle = 0,
	       geometryShaderHandle = 0,
	       tessEvalShaderHandle = 0,
	       tessCtrlShaderHandle = 0,
	       pixelShaderHandle = 0;
	
	GLuint programHandle = glCreateProgram();

	auto result = new ShaderProgram(programHandle);

	auto& keywordMap = result->keywords;

	for (auto& keyword : this->keywordOverrides) {
		keywordMap[keyword.first] = keyword.second;
	}
	
	if (!this->vertexShaderPath.empty()) {
		vertexShader = Shader::LoadFromFile(this->vertexShaderPath);

		result->pragmas.insert(vertexShader.code.pragmas.begin(), vertexShader.code.pragmas.end());

		vertexShaderHandle = CompileShader(vertexShader, this->keywordOverrides, GL_VERTEX_SHADER);

		glAttachShader(programHandle, vertexShaderHandle);

		for (const auto& keyword : vertexShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = keyword.second.defaultValue;
			}
		}
	}
	if (!this->geometryShaderPath.empty()) {
		geometryShader = Shader::LoadFromFile(this->geometryShaderPath);

		result->pragmas.insert(geometryShader.code.pragmas.begin(), geometryShader.code.pragmas.end());

		geometryShaderHandle = CompileShader(geometryShader, this->keywordOverrides, GL_GEOMETRY_SHADER);

		glAttachShader(programHandle, geometryShaderHandle);

		for (const auto& keyword : geometryShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = keyword.second.defaultValue;
			}
		}
	}
	if (!this->tessEvalShaderPath.empty()) {
		tessEvalShader = Shader::LoadFromFile(this->tessEvalShaderPath);

		result->pragmas.insert(tessEvalShader.code.pragmas.begin(), tessEvalShader.code.pragmas.end());

		tessEvalShaderHandle = CompileShader(tessEvalShader, this->keywordOverrides, GL_TESS_EVALUATION_SHADER);

		glAttachShader(programHandle, tessEvalShaderHandle);

		if (tessCtrlShaderHandle) {
			result->pragmas.insert("tesselation");
		}

		for (const auto& keyword : tessEvalShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = keyword.second.defaultValue;
			}
		}
	}
	if (!this->tessCtrlShaderPath.empty()) {
		tessCtrlShader = Shader::LoadFromFile(this->tessCtrlShaderPath);

		result->pragmas.insert(tessCtrlShader.code.pragmas.begin(), tessCtrlShader.code.pragmas.end());

		tessCtrlShaderHandle = CompileShader(tessCtrlShader, this->keywordOverrides, GL_TESS_CONTROL_SHADER);

		glAttachShader(programHandle, tessCtrlShaderHandle);

		for (const auto& keyword : tessCtrlShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = keyword.second.defaultValue;
			}
		}
	}
	if (!this->pixelShaderPath.empty()) {
		pixelShader = Shader::LoadFromFile(this->pixelShaderPath);

		result->pragmas.insert(pixelShader.code.pragmas.begin(), pixelShader.code.pragmas.end());

		pixelShaderHandle = CompileShader(pixelShader, this->keywordOverrides, GL_FRAGMENT_SHADER);

		glAttachShader(programHandle, pixelShaderHandle);

		for (const auto& keyword : pixelShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = keyword.second.defaultValue;
			}
		}
	}

	glLinkProgram(programHandle);
	
	result->vertexShader.shader = vertexShader;
	result->vertexShader.handle = vertexShaderHandle;
	result->geometryShader.shader = geometryShader;
	result->geometryShader.handle = geometryShaderHandle;
	result->tessCtrlShader.shader = tessCtrlShader;
	result->tessCtrlShader.handle = tessCtrlShaderHandle;
	result->tessEvalShader.shader = tessEvalShader;
	result->tessEvalShader.handle = tessEvalShaderHandle;
	result->pixelShader.shader = pixelShader;
	result->pixelShader.handle = pixelShaderHandle;

	result->uniforms = UniformSpec(result);

	return result;
}

ComputeShaderBuilder& ComputeShaderBuilder::WithComputeShader(const fs::path& shaderPath) {
	this->shaderPath = shaderPath;

	return *this;
}

ComputeShaderBuilder& ComputeShaderBuilder::WithKeyword(const std::string& keyword, const std::string& keywordValue) {
	this->keywordOverrides.push_back({keyword, keywordValue});

	return *this;
}
ComputeShaderBuilder& ComputeShaderBuilder::WithKeyword(const std::string& keyword, int keywordValue) {
	this->keywordOverrides.push_back({keyword, std::to_string(keywordValue)});

	return *this;
}
ComputeShaderBuilder& ComputeShaderBuilder::WithKeyword(const std::string& keyword, float keywordValue) {
	this->keywordOverrides.push_back({keyword, std::to_string(keywordValue)});

	return *this;
}

ComputeShaderProgram* ComputeShaderBuilder::Link() {
	GLuint programHandle = glCreateProgram();

	Shader computeShader = Shader::LoadFromFile(this->shaderPath);

	const ShaderCode& code = computeShader.GetCode();

	std::vector<const char*> finalParts(code.codeParts.size());

	for (int i = 0; i < code.codeParts.size(); i++) {
		finalParts[i] = code.codeParts[i];
	}

	for (const auto& keyword : this->keywordOverrides) {
		auto keywordIt = code.keywords.find(keyword.name);

		if (keywordIt != code.keywords.end()) {
			finalParts[keywordIt->second.location] = keyword.value.c_str();
		}
	}

	GLuint shaderHandle = glCreateShader(GL_COMPUTE_SHADER);

	glShaderSource(shaderHandle, finalParts.size(), finalParts.data(), nullptr);

	glCompileShader(shaderHandle);

	GLint compileSuccess;

	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;

		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetShaderInfoLog(shaderHandle, messageLength, &messageLength, infoLog);

		spdlog::error("Error compiling shader {}", fs::canonical(shaderPath).string().c_str());

		std::istringstream shaderLines(infoLog);

		for (std::string line; std::getline(shaderLines, line); ) {
			spdlog::error(std::string(line));
		}

		throw 1;
	}

	glAttachShader(programHandle, shaderHandle);

	glLinkProgram(programHandle);

	glGetProgramiv(programHandle, GL_LINK_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;
		
		glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetProgramInfoLog(shaderHandle, messageLength, &messageLength, infoLog);

		spdlog::error("Error compiling shader {}", shaderPath.string().c_str());
		spdlog::error(std::string(infoLog));

		throw 1;
	}

	return new ComputeShaderProgram(programHandle);
}

ShaderProgram::ShaderProgram(GLuint handle):
keywords(),
vertexShader(),
geometryShader(),
tessEvalShader(),
tessCtrlShader(),
pixelShader(),
uniforms(),
pragmas(),
handle(handle) {
	allPrograms.push_back(this);
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(this->handle);

	std::erase(allPrograms, this);
}

ShaderBuilder ShaderProgram::Build() {
	return ShaderBuilder{};
}

GLuint ShaderProgram::GetHandle() const {
	return this->handle;
}

const UniformSpec& ShaderProgram::GetUniforms() const {
	return this->uniforms;
}

bool ShaderProgram::IgnoresDepthPrepass() const {
	return HasPragma("no_depth_prepass");
}

bool ShaderProgram::CastsShadows() const {
	return !HasPragma("no_shadows");
}

bool ShaderProgram::UsesPatches() const {
	return this->HasPragma("tesselation");
}

bool ShaderProgram::IsTransparent() const {
	return HasPragma("transparent");
}

const Shader ShaderProgram::GetVertexShader() const {
	return this->vertexShader.shader;
}
const Shader ShaderProgram::GetGeometryShader() const {
	return this->geometryShader.shader;
}
const Shader ShaderProgram::GetTessCtrlShader() const {
	return this->tessCtrlShader.shader;
}
const Shader ShaderProgram::GetTessEvalShader() const {
	return this->tessEvalShader.shader;
}
const Shader ShaderProgram::GetPixelShader() const {
	return this->pixelShader.shader;
}

bool ShaderProgram::HasPragma(const std::string& pragma) const {
	return this->pragmas.contains(pragma);
}

void ShaderProgram::Reload() {
	decltype(this->pragmas) newPragmas;
	GLuint newHandle = glCreateProgram();

	ShaderAttachment newVertexShader;
	ShaderAttachment newGeometryShader;
	ShaderAttachment newTessCtrlShader;
	ShaderAttachment newTessEvalShader;
	ShaderAttachment newPixelShader;

	glDeleteProgram(this->handle);
	this->pragmas.clear();

	this->handle = glCreateProgram();

	try {

		if (this->vertexShader.Attached()) {
			// glDeleteShader(this->vertexShader.handle);
	
			newVertexShader.shader = Shader::LoadFromFile(this->vertexShader.shader.GetFilePath());
	
			newPragmas.insert(newVertexShader.shader.GetCode().pragmas.begin(), newVertexShader.shader.GetCode().pragmas.end());
	
			newVertexShader.handle = CompileShader(newVertexShader.shader, this->keywords, GL_VERTEX_SHADER);
	
			glAttachShader(newHandle, newVertexShader.handle);
		}
		if (this->geometryShader.Attached()) {
			// glDeleteShader(this->geometryShader.handle);
	
			newGeometryShader.shader = Shader::LoadFromFile(this->geometryShader.shader.GetFilePath());
	
			newPragmas.insert(newGeometryShader.shader.GetCode().pragmas.begin(), newGeometryShader.shader.GetCode().pragmas.end());
	
			newGeometryShader.handle = CompileShader(newGeometryShader.shader, this->keywords, GL_GEOMETRY_SHADER);
	
			glAttachShader(newHandle, newGeometryShader.handle);
		}
		if (this->tessCtrlShader.Attached()) {
			// glDeleteShader(this->tessCtrlShader.handle);
	
			newTessCtrlShader.shader = Shader::LoadFromFile(this->tessCtrlShader.shader.GetFilePath());
	
			newPragmas.insert(newTessCtrlShader.shader.GetCode().pragmas.begin(), newTessCtrlShader.shader.GetCode().pragmas.end());
	
			newTessCtrlShader.handle = CompileShader(newTessCtrlShader.shader, this->keywords, GL_TESS_CONTROL_SHADER);
	
			glAttachShader(newHandle, newTessCtrlShader.handle);
		}
		if (this->tessEvalShader.Attached()) {
			// glDeleteShader(this->tessEvalShader.handle);
	
			newTessEvalShader.shader = Shader::LoadFromFile(this->tessEvalShader.shader.GetFilePath());
	
			newPragmas.insert(newTessEvalShader.shader.GetCode().pragmas.begin(), newTessEvalShader.shader.GetCode().pragmas.end());
	
			newTessEvalShader.handle = CompileShader(newTessEvalShader.shader, this->keywords, GL_TESS_EVALUATION_SHADER);
	
			glAttachShader(newHandle, newTessEvalShader.handle);
		}
		if (this->pixelShader.Attached()) {
			// glDeleteShader(this->pixelShader.handle);
	
			newPixelShader.shader = Shader::LoadFromFile(this->pixelShader.shader.GetFilePath());
	
			newPragmas.insert(newPixelShader.shader.GetCode().pragmas.begin(), newPixelShader.shader.GetCode().pragmas.end());
	
			newPixelShader.handle = CompileShader(newPixelShader.shader, this->keywords, GL_FRAGMENT_SHADER);
	
			glAttachShader(newHandle, newPixelShader.handle);
		}
	} catch (int error) {
		glDeleteProgram(newHandle);

		if (newVertexShader.handle) {
			glDeleteShader(newVertexShader.handle);
		}
		if (newGeometryShader.handle) {
			glDeleteShader(newGeometryShader.handle);
		}
		if (newTessCtrlShader.handle) {
			glDeleteShader(newTessCtrlShader.handle);
		}
		if (newTessEvalShader.handle) {
			glDeleteShader(newTessEvalShader.handle);
		}
		if (newPixelShader.handle) {
			glDeleteShader(newPixelShader.handle);
		}

		spdlog::error("Error while reloading program");

		return;
	}
	
	glLinkProgram(newHandle);

	int compileSuccess;

	glGetProgramiv(newHandle, GL_LINK_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;
		
		glGetProgramiv(newHandle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetProgramInfoLog(newHandle, messageLength, &messageLength, infoLog);

		spdlog::error("Error linking program");
		spdlog::error(std::string(infoLog));

		return;
	}

	glDeleteProgram(this->handle);

	if (this->vertexShader.handle) {
		glDeleteShader(this->vertexShader.handle);
	}
	if (this->geometryShader.handle) {
		glDeleteShader(this->geometryShader.handle);
	}
	if (this->tessCtrlShader.handle) {
		glDeleteShader(this->tessCtrlShader.handle);
	}
	if (this->tessEvalShader.handle) {
		glDeleteShader(this->tessEvalShader.handle);
	}
	if (this->pixelShader.handle) {
		glDeleteShader(this->pixelShader.handle);
	}
	
	this->handle = newHandle;
	this->pragmas = newPragmas;
	this->uniforms = UniformSpec(this);
}

void ShaderProgram::ReloadAllShaders() {
	for (ShaderProgram* shader : allPrograms) {
		shader->Reload();
	}
}

ComputeShaderProgram::ComputeShaderProgram(GLuint handle) {
	this->handle = handle;

	this->uniforms = UniformSpec(this);
}

ComputeShaderProgram::ComputeShaderProgram(const fs::path& shaderPath) {
	this->handle = glCreateProgram();

	this->computeShader = Shader::LoadFromFile(shaderPath);

	GLuint shaderHandle = glCreateShader(GL_COMPUTE_SHADER);

	glShaderSource(shaderHandle, this->computeShader.GetCode().codeParts.size(), this->computeShader.GetCode().codeParts.data(), nullptr);

	glCompileShader(shaderHandle);

	GLint compileSuccess;

	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;

		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetShaderInfoLog(shaderHandle, messageLength, &messageLength, infoLog);

		spdlog::error("Error compiling shader {}", fs::canonical(shaderPath).string().c_str());

		std::istringstream shaderLines(infoLog);

		for (std::string line; std::getline(shaderLines, line); ) {
			spdlog::error(std::string(line));
		}

		throw 1;
	}

	glAttachShader(this->handle, shaderHandle);

	glLinkProgram(this->handle);

	glGetProgramiv(this->handle, GL_LINK_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;
		
		glGetProgramiv(this->handle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetProgramInfoLog(shaderHandle, messageLength, &messageLength, infoLog);

		spdlog::error("Error compiling shader {}", shaderPath.string().c_str());
		spdlog::error(std::string(infoLog));

		throw 1;
	}

	this->uniforms = UniformSpec(this);
}

ComputeShaderProgram::~ComputeShaderProgram() {
	glDeleteProgram(this->handle);
}

ComputeShaderBuilder ComputeShaderProgram::Build() {
	return ComputeShaderBuilder();
}
ComputeShaderBuilder ComputeShaderProgram::Build(const fs::path& shaderPath) {
	auto result = ComputeShaderBuilder();

	result.WithComputeShader(shaderPath);

	return result;
}

GLuint ComputeShaderProgram::GetHandle() const {
	return this->handle;
}

const UniformSpec& ComputeShaderProgram::GetUniforms() const {
	return this->uniforms;
}

ComputeShaderDispatch::ComputeShaderDispatch(const fs::path& shaderPath):
ComputeShaderDispatch(new ComputeShaderProgram(shaderPath)) { }

ComputeShaderDispatch::ComputeShaderDispatch(ComputeShaderProgram* program) {
	this->program = program;
	this->dispatchData = new ComputeDispatchData(program);
}

void ComputeShaderDispatch::Dispatch(int groupsX, int groupsY, int groupsZ) const {
	this->dispatchData->Bind();

	glDispatchCompute(groupsX, groupsY, groupsZ);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

ComputeDispatchData* ComputeShaderDispatch::GetData() {
	return this->dispatchData;
}
ComputeShaderProgram* ComputeShaderDispatch::GetProgram() {
	return this->program;
}