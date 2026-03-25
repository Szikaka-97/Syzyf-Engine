#include "UniformSpec.h"
#include <Shader.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <ranges>
#include <iostream>
#include <regex>
#include <sstream>
#include <ranges>
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

struct ShaderCodeSegment {
	char* str;
	size_t length;
};

struct ShaderFile {
	fs::path filePath;
	char* content;
};

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
	code.codeParts.push_back(preprocessedSource);
	int partLength = 0;
	int partNum = 0;

	std::stringstream shaderLines(preprocessedSource);

	for (std::string line; std::getline(shaderLines, line); ) {
		auto partStr = code.codeParts.back();

		std::smatch variantMatch{};
		if (std::regex_match(line, variantMatch, Regex::shaderPragmaRegex)) {
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

	// int i = 0;

	// spdlog::info("Shader code:");
	// for (auto part : code.codeParts) {
	// 	std::cout << i++ << '^' << part << '^' << '\n';
	// }
	// for (auto index : code.keywords) {
	// 	spdlog::info("{}: {} = {}", index.second.name, index.second.location, index.second.defaultValue);
	// }
	// for (auto pragma : code.pragmas) {
	// 	spdlog::info("pragma: |{}|", pragma);
	// }

	/*
	spdlog::info("Compiling shader {}", filePath.filename().string());

	int compileSuccess;
	
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);

	if (!compileSuccess) {
		int logLength = 0;
		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLength);
		char* compileMsg = (char*) alloca(sizeof(char) * logLength);
		glGetShaderInfoLog(shaderHandle, logLength, nullptr, compileMsg);

		spdlog::error("Error compiling shader {}:\n{}", filePath.string(), std::string(compileMsg));

		std::istringstream inss(shaderSource);
		std::stringstream outss;

		int lineNum = 1;
		for (std::string line; std::getline(inss, line); ) {
			outss << std::setw(3) << lineNum++ << "| " << line << "\n";
		}

		spdlog::error("Shader source: \n{}", outss.str());

		throw "Nuh uh";

		// throw shader::shader_compilation_exception(path_to_file, compile_msg);
	}
	*/

	Shader result(filePath, code);

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
	this->keywordOverrides.push_back({keyword, keywordValue});

	return *this;
}
ShaderBuilder& ShaderBuilder::WithKeyword(const std::string& keyword, int keywordValue) {
	this->keywordOverrides.push_back({keyword, std::to_string(keywordValue)});

	return *this;
}
ShaderBuilder& ShaderBuilder::WithKeyword(const std::string& keyword, float keywordValue) {
	this->keywordOverrides.push_back({keyword, std::to_string(keywordValue)});

	return *this;
}

GLuint ShaderBuilder::CompileShader(const ShaderCode& code, std::unordered_set<std::string>& pragmas, GLenum shaderType) {
	std::vector<const char*> finalParts(code.codeParts.size());
	std::stringstream ss;

	
	for (int i = 0; i < code.codeParts.size(); i++) {
		finalParts[i] = code.codeParts[i];
	}

	for (const auto& keyword : this->keywordOverrides) {
		auto keywordIt = code.keywords.find(keyword.name);

		if (keywordIt != code.keywords.end()) {
			finalParts[keywordIt->second.location] = keyword.value.c_str();
		}
	}

	for (const std::string& pragma : code.pragmas) {
		pragmas.insert(pragma);
	}

	for (const char* part : finalParts) {
		ss << part;
	}

	std::string s = ss.str();
	const char* finalCode = s.c_str();

	// return glCreateShaderProgramv(shaderType, 1, &finalCode);
	GLuint handle = glCreateShader(shaderType);

	glShaderSource(handle, 1, &finalCode, nullptr);

	glCompileShader(handle);

	GLint compileSuccess;

	glGetShaderiv(handle, GL_COMPILE_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;

		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetShaderInfoLog(handle, messageLength, &messageLength, infoLog);

		spdlog::error(std::string(infoLog));
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
	
	std::unordered_set<std::string> pragmas;

	if (!this->vertexShaderPath.empty()) {
		vertexShader = Shader::LoadFromFile(this->vertexShaderPath);

		vertexShaderHandle = CompileShader(vertexShader.GetCode(), pragmas, GL_VERTEX_SHADER);
	}
	if (!this->geometryShaderPath.empty()) {
		geometryShader = Shader::LoadFromFile(this->geometryShaderPath);

		geometryShaderHandle = CompileShader(geometryShader.GetCode(), pragmas, GL_GEOMETRY_SHADER);
	}
	if (!this->tessEvalShaderPath.empty()) {
		tessEvalShader = Shader::LoadFromFile(this->tessEvalShaderPath);

		tessEvalShaderHandle = CompileShader(tessEvalShader.GetCode(), pragmas, GL_TESS_EVALUATION_SHADER);
	}
	if (!this->tessCtrlShaderPath.empty()) {
		tessCtrlShader = Shader::LoadFromFile(this->tessCtrlShaderPath);

		tessCtrlShaderHandle = CompileShader(tessCtrlShader.GetCode(), pragmas, GL_TESS_CONTROL_SHADER);
	}
	if (!this->pixelShaderPath.empty()) {
		pixelShader = Shader::LoadFromFile(this->pixelShaderPath);

		pixelShaderHandle = CompileShader(pixelShader.GetCode(), pragmas, GL_FRAGMENT_SHADER);
	}

	GLuint programHandle = glCreateProgram();

	auto result = new ShaderProgram(programHandle);
	
	result->keywords.variants.push_back({});

	auto& keywordMap = result->keywords.keywordMap;
	auto& defaultVariant = result->keywords.variants.back();

	if (vertexShaderHandle) {
		glAttachShader(programHandle, vertexShaderHandle);

		result->pragmas.insert(vertexShader.GetCode().pragmas.begin(), vertexShader.GetCode().pragmas.end());

		for (const auto& keyword : vertexShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = defaultVariant.keywords.size();
				defaultVariant.keywords.push_back(keyword.second.defaultValue);
			}
		}

		defaultVariant.shaders.vertexShaderHandle = vertexShaderHandle;
	}
	if (geometryShaderHandle) {
		glAttachShader(programHandle, geometryShaderHandle);

		result->pragmas.insert(geometryShader.GetCode().pragmas.begin(), geometryShader.GetCode().pragmas.end());

		for (const auto& keyword : geometryShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = defaultVariant.keywords.size();
				defaultVariant.keywords.push_back(keyword.second.defaultValue);
			}
		}

		defaultVariant.shaders.geometryShaderHandle = geometryShaderHandle;
	}
	if (tessEvalShaderHandle) {
		glAttachShader(programHandle, tessEvalShaderHandle);

		result->pragmas.insert(tessEvalShader.GetCode().pragmas.begin(), tessEvalShader.GetCode().pragmas.end());

		for (const auto& keyword : tessEvalShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = defaultVariant.keywords.size();
				defaultVariant.keywords.push_back(keyword.second.defaultValue);
			}
		}

		defaultVariant.shaders.tessEvalShaderHandle = tessEvalShaderHandle;

		if (tessCtrlShaderHandle) {
			result->pragmas.insert("tesselation");
		}
	}
	if (tessCtrlShaderHandle) {
		glAttachShader(programHandle, tessCtrlShaderHandle);

		result->pragmas.insert(tessCtrlShader.GetCode().pragmas.begin(), tessCtrlShader.GetCode().pragmas.end());

		for (const auto& keyword : tessCtrlShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = defaultVariant.keywords.size();
				defaultVariant.keywords.push_back(keyword.second.defaultValue);
			}
		}

		defaultVariant.shaders.tessCtrlShaderHandle = tessCtrlShaderHandle;
	}
	if (pixelShaderHandle) {
		glAttachShader(programHandle, pixelShaderHandle);

		result->pragmas.insert(pixelShader.GetCode().pragmas.begin(), pixelShader.GetCode().pragmas.end());

		for (const auto& keyword : pixelShader.GetCode().keywords) {
			if (!keywordMap.contains(keyword.first)) {
				keywordMap[keyword.first] = defaultVariant.keywords.size();
				defaultVariant.keywords.push_back(keyword.second.defaultValue);
			}
		}

		defaultVariant.shaders.pixelShaderHandle = pixelShaderHandle;
	}

	glLinkProgram(programHandle);
	
	result->uniforms = UniformSpec(result);

	return result;
}

ShaderProgram::ShaderProgram(GLuint handle):
keywords(),
currentVariant(nullptr),
vertexShader(),
geometryShader(),
tessEvalShader(),
tessCtrlShader(),
pixelShader(),
uniforms(),
pragmas(),
handle(handle) { }

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(this->handle);
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

bool ShaderProgram::HasPragma(const std::string& pragma) const {
	return this->pragmas.contains(pragma);
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

		spdlog::error(std::string(infoLog));
	}

	glAttachShader(this->handle, shaderHandle);

	glLinkProgram(this->handle);

	glGetProgramiv(this->handle, GL_LINK_STATUS, &compileSuccess);

	if (!compileSuccess) {
		GLint messageLength;
		
		glGetProgramiv(this->handle, GL_INFO_LOG_LENGTH, &messageLength);

		char* infoLog = new char[messageLength];

		glGetProgramInfoLog(shaderHandle, messageLength, &messageLength, infoLog);

		spdlog::error(std::string(infoLog));
	}

	this->uniforms = UniformSpec(this);	
}

ComputeShaderProgram::~ComputeShaderProgram() {
	glDeleteProgram(this->handle);
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