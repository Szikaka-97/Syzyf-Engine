#include <Shader.h>

#include <fstream>

#include <PreComp.h>

#include <spdlog/spdlog.h>

ShaderVariantInfo::ShaderVariantInfo(std::initializer_list<ShaderVariantPoint> variantPoints):
variantPoints(variantPoints) { }

ShaderBase::ShaderBase(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
filePath(filePath),
variantInfo(variantInfo),
handle(handle) { }

const std::string versionHeader =
"#version 460\n"
;

const std::string definesHeader = 
"#define IN_POSITION location=0\n"
"#define IN_NORMAL location=1\n"
"#define IN_BINORMAL location=2\n"
"#define IN_TANGENT location=3\n"
"#define IN_UV1 location=4\n"
"#define IN_UV2 location=5\n"
"#define IN_COLOR location=6\n"
;

const std::string builtinUniformsHeader =
"struct Light {\n"
"	vec4 positionAndType;\n"
"	vec4 directionAndFov;\n"
"	vec4 colorAndStrength;\n"
"};\n"
"layout (std140, binding = 0) uniform GlobalUniforms\n"
"{\n"
"	mat4 Global_ViewMatrix;\n"
"	mat4 Global_ProjectionMatrix;\n"
"	mat4 Global_VPMatrix;\n"
"	Light Global_LightInfo;\n"
"	float Global_Time;\n"
"};\n"
"layout (std140, binding = 1) uniform ObjectUniforms\n"
"{\n"
"	mat4 Object_ModelMatrix;\n"
"	mat4 Object_MVPMatrix;\n"
"};\n"
;

char* LoadFile(const fs::path& filePath, GLenum& shaderType) {
	fs::directory_entry shaderFile(filePath);

	// if (!shaderFile.exists()) {
	// 	throw shader::shader_missing_file_exception(shaderFile);
	// }

	shaderType = GL_INVALID_ENUM;
	
	if (filePath.extension() == ".vert") {
		shaderType = GL_VERTEX_SHADER;
	}
	else if (filePath.extension() == ".frag") {
		shaderType = GL_FRAGMENT_SHADER;
	}
	else if (filePath.extension() == ".geom") {
		shaderType = GL_GEOMETRY_SHADER;
	}
	else {
		// throw shader::shader_unknown_type_exception(path_to_file);
	}

	char* buf = new char[shaderFile.file_size() + 1];
	
	std::ifstream shaderFileStream(filePath);
	
	shaderFileStream.read(buf, shaderFile.file_size());
	
	buf[shaderFile.file_size()] = '\0';
	
	return buf;
}

int VersionPresent(const std::string& shaderCode) {
	std::smatch versionMatch;

	if (std::regex_search(shaderCode.begin(), shaderCode.end(), versionMatch, Regex::shaderHeaderRegex)) {
		return versionMatch.position() + versionMatch.length();
	}

	return 0;
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

	for (const auto& input : spec) {
		spdlog::info("Input: {}, {}", VertexSpec::TypeToName(input.type), input.length);
	}

	return VertexSpec(spec);
}

ShaderBase* ShaderBase::Load(fs::path filePath) {
	GLenum shaderType;
	char* buf = LoadFile(filePath, shaderType);
	std::string shaderCode(buf);
	GLuint shaderHandle = glCreateShader(shaderType);

	int versionOffset = VersionPresent(shaderCode);

	if (shaderType == GL_VERTEX_SHADER) {
		const char* shaderCodeParts[4];
		int shaderCodeLengths[4] {-1, -1, -1, -1};
	
		if (versionOffset > 0) {
			shaderCodeParts[0] = buf;
			shaderCodeLengths[0] = versionOffset;
		}
		else {
			shaderCodeParts[0] = versionHeader.c_str();
		}
	
		shaderCodeParts[1] = definesHeader.c_str();
		shaderCodeParts[2] = builtinUniformsHeader.c_str();
		shaderCodeParts[3] = buf + versionOffset;
	
		glShaderSource(shaderHandle, 4, shaderCodeParts, shaderCodeLengths);
	}
	else {
		const char* shaderCodeParts[3];
		int shaderCodeLengths[3] {-1, -1, -1};
	
		if (versionOffset > 0) {
			shaderCodeParts[0] = buf;
			shaderCodeLengths[0] = versionOffset;
		}
		else {
			shaderCodeParts[0] = versionHeader.c_str();
		}
	
		shaderCodeParts[1] = builtinUniformsHeader.c_str();
		shaderCodeParts[2] = buf + versionOffset;
	
		glShaderSource(shaderHandle, 3, shaderCodeParts, shaderCodeLengths);
	}
	
	glCompileShader(shaderHandle);

	delete[] buf;

	int compileSuccess;
	char compileMsg[512];

	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);

	if (!compileSuccess) {
		glGetShaderInfoLog(shaderHandle, 512, NULL, compileMsg);

		spdlog::error("Error compiling shader {}:\n{}", filePath.string(), compileMsg);

		int sourceLength = 0;
		char shaderSource[4096];

		glGetShaderSource(shaderHandle, 4096, &sourceLength, shaderSource);

		spdlog::error("Shader source: \n{}", shaderSource);

		return nullptr;

		// throw shader::shader_compilation_exception(path_to_file, compile_msg);
	}

	spdlog::info("Compiled shader {}", filePath.filename().string());

	ShaderBase* result;

	if (shaderType == GL_VERTEX_SHADER) {
		VertexSpec spec = GetVertexSpec(shaderCode);

		result = new VertexShader(filePath, {}, shaderHandle, spec);
	}
	else if (shaderType == GL_FRAGMENT_SHADER) {
		result = new PixelShader(filePath, {}, shaderHandle);
	}
	else if (shaderType == GL_GEOMETRY_SHADER) {
		result = new GeometryShader(filePath, {}, shaderHandle);
	}
	else {
		// throw shader::shader_unknown_type_exception(path_to_file);
	}

	return result;
}

ShaderBase* ShaderBase::Load(fs::path filePath, const ShaderVariantInfo& variantInfo) {
#warning TODO
	return nullptr;
}

const fs::path& ShaderBase::GetFilePath() const {
	return this->filePath;
}

std::string ShaderBase::GetName() const {
	return this->filePath.stem().string();
}
const ShaderVariantInfo& ShaderBase::GetVariantInfo() const {
	return this->variantInfo;
}
GLuint ShaderBase::GetHandle() const {
	return this->handle;
}

VertexShader::VertexShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle, VertexSpec spec):
ShaderBase(filePath, variantInfo, handle),
vertexSpec(spec) { }

const VertexSpec& VertexShader::GetVertexSpec() const {
	return this->vertexSpec;
}

GLenum VertexShader::GetType() const {
	return GL_VERTEX_SHADER;
}

GeometryShader::GeometryShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
ShaderBase(filePath, variantInfo, handle) { }

GLenum GeometryShader::GetType() const {
	return GL_GEOMETRY_SHADER;
}

PixelShader::PixelShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
ShaderBase(filePath, variantInfo, handle) { }

GLenum PixelShader::GetType() const {
	return GL_FRAGMENT_SHADER;
}

ShaderBuilder& ShaderBuilder::WithVertexShader(VertexShader* vertexShader) {
	this->vertexShader = vertexShader;

	return *this;
}

ShaderBuilder& ShaderBuilder::WithGeometryShader(GeometryShader* geometryShader) {
	this->geometryShader = geometryShader;

	return *this;
}

ShaderBuilder& ShaderBuilder::WithPixelShader(PixelShader* pixelShader) {
	this->pixelShader = pixelShader;

	return *this;
}

ShaderProgram* ShaderBuilder::Link() {
	GLuint programHandle = glCreateProgram();

	assert(this->vertexShader);
	assert(this->pixelShader);

	if (this->vertexShader) {
		glAttachShader(programHandle, this->vertexShader->GetHandle());
	}

	if (this->geometryShader) {
		glAttachShader(programHandle, this->geometryShader->GetHandle());
	}

	if (this->pixelShader) {
		glAttachShader(programHandle, this->pixelShader->GetHandle());
	}

	glLinkProgram(programHandle);

	int compileSuccess;
	char compileMsg[512];

	glGetProgramiv(programHandle, GL_LINK_STATUS, &compileSuccess);
	if (!compileSuccess) {
		glGetProgramInfoLog(programHandle, 512, NULL, compileMsg);

		spdlog::error("Error linking shader:\n{}", compileMsg);
	}

	return new ShaderProgram(
		this->vertexShader,
		this->geometryShader,
		this->pixelShader,
		programHandle
	);
}

bool IsUniformTypeSupported(GLenum type) {
	return (
		type == GL_UNSIGNED_INT
		||
		type == GL_FLOAT
		||
		(type >= GL_FLOAT_VEC2 && type <= GL_FLOAT_VEC4)
		||
		(type >= GL_UNSIGNED_INT_VEC2 && type <= GL_UNSIGNED_INT_VEC4)
		||
		type == GL_FLOAT_MAT3
		||
		type == GL_FLOAT_MAT4
		||
		type == GL_SAMPLER_2D
	);
}

UniformType GLEnumToUniformType(GLenum type) {
	const static std::map<GLenum, UniformType> dict {
		{ GL_FLOAT, UniformType::Float1 },
		{ GL_FLOAT_VEC2, UniformType::Float2 },
		{ GL_FLOAT_VEC3, UniformType::Float3 },
		{ GL_FLOAT_VEC4, UniformType::Float4 },
		{ GL_UNSIGNED_INT, UniformType::Uint1 },
		{ GL_UNSIGNED_INT_VEC2, UniformType::Uint2 },
		{ GL_UNSIGNED_INT_VEC3, UniformType::Uint3 },
		{ GL_UNSIGNED_INT_VEC4, UniformType::Uint4 },
		{ GL_FLOAT_MAT3, UniformType::Matrix3x3 },
		{ GL_FLOAT_MAT4, UniformType::Matrix4x4 },
		{ GL_SAMPLER_2D, UniformType::Sampler2D },
	};

	return dict.at(type);
}

ShaderProgram::ShaderProgram(VertexShader* vertexShader, GeometryShader* geometryShader, PixelShader* pixelShader, GLuint handle):
vertexShader(vertexShader),
geometryShader(geometryShader),
pixelShader(pixelShader),
handle(handle) {
	int uniformCount = 0;
	glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniformCount);

	int bufferSize = 0;
	glGetProgramiv(handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufferSize);
	char uniformName[bufferSize + 1];

	std::vector<UniformVariable> uniforms;
	std::vector<TextureVariable> textures;

	for (int i = 0; i < uniformCount; i++) {
		int nameLength = 0;
		int uniformSize = 0;
		GLenum uniformType;
		glGetActiveUniform(handle, i, bufferSize, &nameLength, &uniformSize, &uniformType, uniformName);

		if (IsUniformTypeSupported(uniformType) && !std::string(uniformName).starts_with("Object_") && !std::string(uniformName).starts_with("Global_")) {
			uniforms.push_back({ GLEnumToUniformType(uniformType), std::string(uniformName) });
		}
		else {
			uniforms.push_back({ UniformType::Unsupported, uniformName });
		}

		spdlog::info("name: {}, index: {}, location: {}, type: 0x{:x}", std::string(uniformName), i, glGetUniformLocation(handle, uniformName), (int) uniformType);
	}

	this->uniforms = UniformSpec(uniforms);

	// for (int i = 0; i < uniformCount; i++) {
	// 	spdlog::info("Uniform at {}: name = {}, type = {}, offset = {}", i, this->uniforms.variables[i].name, (int) this->uniforms.variables[i].type, this->uniforms.offsets[i]);
	// }

	spdlog::info("Total buffer size: {}", this->uniforms.GetBufferSize());
}

ShaderBuilder ShaderProgram::Build() {
	return ShaderBuilder{};
}

GLuint ShaderProgram::GetHandle() {
	return this->handle;
}

const UniformSpec& ShaderProgram::GetUniforms() const {
	return this->uniforms;
}
const VertexSpec& ShaderProgram::GetVertexSpec() const {
	return this->vertexShader->GetVertexSpec();
}