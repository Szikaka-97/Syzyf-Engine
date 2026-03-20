#include <Shader.h>

#include <cstddef>
#include <cstring>
#include <fstream>
#include <ranges>
#include <sstream>
#include <ranges>
#include <malloc.h>
#include <stb_include.h>

#include <PreComp.h>
#include <Material.h>

#include <spdlog/spdlog.h>
#include <string_view>

struct ShaderCodeSegment {
	char* str;
	size_t length;
};

struct ShaderFile {
	fs::path filePath;
	char* content;
};

struct ShaderCode {
	std::vector<ShaderFile> loadedFiles;
	std::vector<ShaderCodeSegment> segments;
};

ShaderVariantInfo::ShaderVariantInfo(std::initializer_list<ShaderVariantPoint> variantPoints):
variantPoints(variantPoints) { }

ShaderBase::ShaderBase(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
filePath(filePath),
variantInfo(variantInfo),
handle(handle) { }

ShaderBase::~ShaderBase() {
	glDeleteShader(this->handle);
}

char* LoadFile(const fs::path& filePath) {
	fs::directory_entry shaderFile(filePath);

	// if (!shaderFile.exists()) {
	// 	throw shader::shader_missing_file_exception(shaderFile);
	// }

	char* buf = new char[shaderFile.file_size() + 1];
	
	std::ifstream shaderFileStream(filePath, std::ios::binary);

	shaderFileStream.read(buf, shaderFile.file_size());
	
	buf[shaderFile.file_size()] = '\0';
	
	return buf;
}

char* LoadFile(const fs::path& filePath, GLenum& shaderType) {
	fs::directory_entry shaderFile(filePath);

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
	else if (filePath.extension() == ".tess_eval") {
		shaderType = GL_TESS_EVALUATION_SHADER;
	}
	else if (filePath.extension() == ".tess_ctrl") {
		shaderType = GL_TESS_CONTROL_SHADER;
	}
	else if (filePath.extension() == ".comp") {
		shaderType = GL_COMPUTE_SHADER;
	}
	else {
		// throw shader::shader_unknown_type_exception(path_to_file);
	}

	return LoadFile(filePath);
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

	return VertexSpec(spec);
}

ShaderBase* ShaderBase::Load(fs::path filePath) {
	GLenum shaderType;

	if (filePath.extension() == ".vert") {
		shaderType = GL_VERTEX_SHADER;
	}
	else if (filePath.extension() == ".frag") {
		shaderType = GL_FRAGMENT_SHADER;
	}
	else if (filePath.extension() == ".geom") {
		shaderType = GL_GEOMETRY_SHADER;
	}
	else if (filePath.extension() == ".tess_eval") {
		shaderType = GL_TESS_EVALUATION_SHADER;
	}
	else if (filePath.extension() == ".tess_ctrl") {
		shaderType = GL_TESS_CONTROL_SHADER;
	}
	else if (filePath.extension() == ".comp") {
		shaderType = GL_COMPUTE_SHADER;
	}
	else {
		throw "unknown extention";
	}

	GLuint shaderHandle = glCreateShader(shaderType);

	char preprocessorErrorMsg[256];

	char* shaderSource = LoadFile(filePath);

	char* preprocessedSource = stb_include_string(shaderSource, (char* ) "", (char* ) "./res/shaders", filePath.string().data(), preprocessorErrorMsg);

	if (preprocessedSource == nullptr) {
		spdlog::error("Error preprocessing shader {}:\n{}", filePath.string(), std::string(preprocessorErrorMsg));

		return nullptr;
	}

	glShaderSource(shaderHandle, 1, &preprocessedSource, nullptr);

	spdlog::info("Compiling shader {}", filePath.filename().string());

	glCompileShader(shaderHandle);

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

	ShaderBase* result;

	if (shaderType == GL_VERTEX_SHADER) {
		VertexSpec spec = GetVertexSpec(preprocessedSource);

		result = new VertexShader(filePath, {}, shaderHandle, spec);
	}
	else if (shaderType == GL_FRAGMENT_SHADER) {
		result = new PixelShader(filePath, {}, shaderHandle);
	}
	else if (shaderType == GL_GEOMETRY_SHADER) {
		result = new GeometryShader(filePath, {}, shaderHandle);
	}
	else if (filePath.extension() == ".tess_eval") {
		result = new TesselationEvaluationShader(filePath, {}, shaderHandle);
	}
	else if (filePath.extension() == ".tess_ctrl") {
		result = new TesselationControlShader(filePath, {}, shaderHandle);
	}
	else if (shaderType == GL_COMPUTE_SHADER) {
		result = new ComputeShader(filePath, {}, shaderHandle);
	}
	else {
		// throw shader::shader_unknown_type_exception(path_to_file);
	}

	free(preprocessedSource);

	return result;
}

ShaderBase* ShaderBase::Load(fs::path filePath, const ShaderVariantInfo& variantInfo) {
#ifndef _MSC_VER
#warning TODO
#endif
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

VertexShader* VertexShader::Load(fs::path filePath) {
	ShaderBase* loaded = ShaderBase::Load(filePath);

	VertexShader* result = dynamic_cast<VertexShader*>(loaded);

	if (!result) {
		delete loaded;

		return nullptr;
	}
	
	return result;
}

const VertexSpec& VertexShader::GetVertexSpec() const {
	return this->vertexSpec;
}

GLenum VertexShader::GetType() const {
	return GL_VERTEX_SHADER;
}

GeometryShader::GeometryShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
ShaderBase(filePath, variantInfo, handle) { }

GeometryShader* GeometryShader::Load(fs::path filePath) {
	ShaderBase* loaded = ShaderBase::Load(filePath);

	GeometryShader* result = dynamic_cast<GeometryShader*>(loaded);

	if (!result) {
		delete loaded;

		return nullptr;
	}
	
	return result;
}

GLenum GeometryShader::GetType() const {
	return GL_GEOMETRY_SHADER;
}

TesselationEvaluationShader::TesselationEvaluationShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
ShaderBase(filePath, variantInfo, handle) { }

TesselationEvaluationShader* TesselationEvaluationShader::Load(fs::path filePath) {
	ShaderBase* loaded = ShaderBase::Load(filePath);

	TesselationEvaluationShader* result = dynamic_cast<TesselationEvaluationShader*>(loaded);

	if (!result) {
		delete loaded;

		return nullptr;
	}
	
	return result;
}

GLenum TesselationEvaluationShader::GetType() const {
	return GL_TESS_EVALUATION_SHADER;
}

TesselationControlShader::TesselationControlShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
ShaderBase(filePath, variantInfo, handle) { }

TesselationControlShader* TesselationControlShader::Load(fs::path filePath) {
	ShaderBase* loaded = ShaderBase::Load(filePath);

	TesselationControlShader* result = dynamic_cast<TesselationControlShader*>(loaded);

	if (!result) {
		delete loaded;

		return nullptr;
	}
	
	return result;
}

GLenum TesselationControlShader::GetType() const {
	return GL_TESS_CONTROL_SHADER;
}

PixelShader::PixelShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
ShaderBase(filePath, variantInfo, handle) { }

PixelShader* PixelShader::Load(fs::path filePath) {
	ShaderBase* loaded = ShaderBase::Load(filePath);

	PixelShader* result = dynamic_cast<PixelShader*>(loaded);

	if (!result) {
		delete loaded;

		return nullptr;
	}
	
	return result;
}

GLenum PixelShader::GetType() const {
	return GL_FRAGMENT_SHADER;
}

ComputeShader::ComputeShader(fs::path filePath, ShaderVariantInfo variantInfo, GLuint handle):
ShaderBase(filePath, variantInfo, handle) { }

ComputeShader* ComputeShader::Load(fs::path filePath) {
	ShaderBase* loaded = ShaderBase::Load(filePath);

	ComputeShader* result = dynamic_cast<ComputeShader*>(loaded);

	if (!result) {
		delete loaded;

		return nullptr;
	}
	
	return result;
}

GLenum ComputeShader::GetType() const {
	return GL_COMPUTE_SHADER;
}

ShaderBuilder& ShaderBuilder::WithVertexShader(VertexShader* vertexShader) {
	this->vertexShader = vertexShader;

	return *this;
}

ShaderBuilder& ShaderBuilder::WithGeometryShader(GeometryShader* geometryShader) {
	this->geometryShader = geometryShader;

	return *this;
}

ShaderBuilder& ShaderBuilder::WithTessEvaluationShader(TesselationEvaluationShader* tessEvalShader) {
	this->tessEvalShader = tessEvalShader;

	return *this;
}
ShaderBuilder& ShaderBuilder::WithTessControlShader(TesselationControlShader* tessCtrlShader) {
	this->tessCtrlShader = tessCtrlShader;

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

	glAttachShader(programHandle, this->vertexShader->GetHandle());
	glAttachShader(programHandle, this->pixelShader->GetHandle());
	
	if (this->geometryShader) {
		glAttachShader(programHandle, this->geometryShader->GetHandle());
	}
	
	if (this->tessCtrlShader && this->tessEvalShader) {
		glAttachShader(programHandle, this->tessEvalShader->GetHandle());
		glAttachShader(programHandle, this->tessCtrlShader->GetHandle());
	}

	glLinkProgram(programHandle);

	int compileSuccess;
	char compileMsg[512];

	glGetProgramiv(programHandle, GL_LINK_STATUS, &compileSuccess);
	if (!compileSuccess) {
		glGetProgramInfoLog(programHandle, 512, NULL, compileMsg);

		spdlog::error("Error linking shader:\n{}", compileMsg);
	}

	ShaderProgram* prog = new ShaderProgram(
		this->vertexShader,
		this->geometryShader,
		this->pixelShader,
		programHandle
	);

	if (this->tessCtrlShader && this->tessEvalShader) {
		prog->flags = ShaderProgramFlags::UsePatches;
	}
	else {
		prog->flags = ShaderProgramFlags::None;
	}

	return prog;
}

ShaderProgram::ShaderProgram(VertexShader* vertexShader, GeometryShader* geometryShader, PixelShader* pixelShader, GLuint handle):
vertexShader(vertexShader),
geometryShader(geometryShader),
pixelShader(pixelShader),
handle(handle) {
	this->uniforms = UniformSpec(this);
}

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
const VertexSpec& ShaderProgram::GetVertexSpec() const {
	return this->vertexShader->GetVertexSpec();
}

bool ShaderProgram::IgnoresDepthPrepass() const {
	return int(this->flags & ShaderProgramFlags::IgnoreDepthPrepass) != 0;
}

bool ShaderProgram::CastsShadows() const {
	return int(this->flags & ShaderProgramFlags::DontCastShadows) == 0;
}

bool ShaderProgram::UsesPatches() const {
	return int(this->flags & ShaderProgramFlags::UsePatches) != 0;
}

bool ShaderProgram::IsTransparent() const {
	return int(this->flags & ShaderProgramFlags::Transparent) != 0;
}

void ShaderProgram::SetIgnoresDepthPrepass(bool ignores) {
	unsigned int temp = (unsigned int) ShaderProgramFlags::IgnoreDepthPrepass;
	temp = ~temp;

	temp = (unsigned int) this->flags & temp;

	temp |= (unsigned int) ShaderProgramFlags::IgnoreDepthPrepass * ignores;

	this->flags = (ShaderProgramFlags) temp;
}

void ShaderProgram::SetCastsShadows(bool casts) {
	unsigned int temp = (unsigned int) ShaderProgramFlags::DontCastShadows;
	temp = ~temp;

	temp = (unsigned int) this->flags & temp;

	temp |= (unsigned int) ShaderProgramFlags::DontCastShadows * !casts;

	this->flags = (ShaderProgramFlags) temp;
}

void ShaderProgram::SetTransparent(bool transparent) {
	unsigned int temp = (unsigned int) ShaderProgramFlags::Transparent;
	temp = ~temp;

	temp = (unsigned int) this->flags & temp;

	temp |= (unsigned int) ShaderProgramFlags::Transparent * transparent;

	this->flags = (ShaderProgramFlags) temp;
}

ComputeShaderProgram::ComputeShaderProgram(ComputeShader* computeShader) {
	assert(computeShader);

	this->handle = glCreateProgram();
	glAttachShader(this->handle, computeShader->GetHandle());

	glLinkProgram(this->handle);

	int compileSuccess;
	char compileMsg[512];

	glGetProgramiv(this->handle, GL_LINK_STATUS, &compileSuccess);
	if (!compileSuccess) {
		glGetProgramInfoLog(this->handle, 512, NULL, compileMsg);

		spdlog::error("Error linking compute shader program:\n{}", compileMsg);
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

ComputeShaderDispatch::ComputeShaderDispatch(ComputeShader* compShader):
ComputeShaderDispatch(new ComputeShaderProgram(compShader)) { }

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