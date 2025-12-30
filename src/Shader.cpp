#include <Shader.h>

#include <fstream>
#include <sstream>
#include <queue>

#include <PreComp.h>
#include <Material.h>

#include <spdlog/spdlog.h>

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
	
	std::ifstream shaderFileStream(filePath);
	
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
	else if (filePath.extension() == ".comp") {
		shaderType = GL_COMPUTE_SHADER;
	}
	else {
		// throw shader::shader_unknown_type_exception(path_to_file);
	}

	GLuint shaderHandle = glCreateShader(shaderType);
	ShaderCode code;

	std::queue<fs::path> filesToLoad;
	filesToLoad.push(filePath);

	while (!filesToLoad.empty()) {
		fs::path loadedFilePath = filesToLoad.front();
		filesToLoad.pop();

		if (std::any_of(code.loadedFiles.begin(), code.loadedFiles.end(), [loadedFilePath](const ShaderFile& f) -> bool {
			return f.filePath == loadedFilePath;
		} )) {
			continue;
		}

		ShaderFile loadedFile;
		loadedFile.filePath = loadedFilePath;
		loadedFile.content = LoadFile(loadedFilePath);

		code.loadedFiles.push_back(loadedFile);

		std::string_view codeView(loadedFile.content, strlen(loadedFile.content));
		auto codeIt = std::regex_iterator(codeView.cbegin(), codeView.cend(), Regex::shaderIncludeRegex);

		for (decltype(codeIt) last; codeIt != last; ++codeIt) {
			fs::path includedFile = BaseShaderPath / (*codeIt)[1].str();
			filesToLoad.push(includedFile);
		}
	}

	code.segments.push_back({code.loadedFiles[0].content, strlen(code.loadedFiles[0].content)});
	std::vector<fs::path> expandedFiles;
	expandedFiles.push_back(filePath);

	bool trip = false;
	while (expandedFiles.size() != code.loadedFiles.size()) {
		std::vector<ShaderCodeSegment> newCodeSegments;

		for (const auto& segment : code.segments) {
			size_t pointer = 0;
			std::string_view codeView(segment.str, segment.length);
			auto codeIt = std::regex_iterator(codeView.cbegin(), codeView.cend(), Regex::shaderIncludeRegex);

			for (decltype(codeIt) last; codeIt != last; ++codeIt) {
				const auto& match = *codeIt;

				newCodeSegments.push_back({segment.str + pointer, match.position() - pointer});
				
				pointer = match.position() + match.length();

				fs::path includedFilePath = BaseShaderPath / match[1].str();

				if (!std::any_of(expandedFiles.begin(), expandedFiles.end(), [includedFilePath](const fs::path& p) -> bool {
					return p == includedFilePath;
				} )) {
					const auto& includedFile = *std::find_if(code.loadedFiles.begin(), code.loadedFiles.end(), [includedFilePath](const ShaderFile& f) -> bool {
						return f.filePath == includedFilePath;
					} );

					newCodeSegments.push_back({includedFile.content, strlen(includedFile.content)});
					expandedFiles.push_back(includedFilePath);
				}
			}

			newCodeSegments.push_back({segment.str + pointer, segment.length - pointer});
		}

		code.segments = newCodeSegments;
	}

	char* segmentsStrings[code.segments.size()];
	int segmentsLengths[code.segments.size()];

	for (int i = 0; i < code.segments.size(); i++) {
		segmentsStrings[i] = code.segments[i].str;
		segmentsLengths[i] = code.segments[i].length;
	}

	glShaderSource(shaderHandle, code.segments.size(), segmentsStrings, segmentsLengths);

	glCompileShader(shaderHandle);

	int compileSuccess;
	
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);

	if (!compileSuccess) {
		int logLength = 0;
		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLength);
		char compileMsg[logLength];
		glGetShaderInfoLog(shaderHandle, logLength, nullptr, compileMsg);

		spdlog::error("Error compiling shader {}:\n{}", filePath.string(), std::string(compileMsg));

		int sourceLength = 0;
		glGetShaderiv(shaderHandle, GL_SHADER_SOURCE_LENGTH, &sourceLength);
		char shaderSource[sourceLength];

		glGetShaderSource(shaderHandle, sourceLength, nullptr, shaderSource);

		std::istringstream inss(shaderSource);
		std::stringstream outss;

		int lineNum = 1;
		for (std::string line; std::getline(inss, line); ) {
			outss << std::setw(3) << lineNum++ << "| " << line << "\n";
		}

		spdlog::error("Shader source: \n{}", outss.str());

		*((int *) 0) = 0;
		
		return nullptr;

		// throw shader::shader_compilation_exception(path_to_file, compile_msg);
	}

	spdlog::info("Compiled shader {}", filePath.filename().string());

	ShaderBase* result;

	if (shaderType == GL_VERTEX_SHADER) {
		VertexSpec spec = GetVertexSpec(code.loadedFiles[0].content);

		result = new VertexShader(filePath, {}, shaderHandle, spec);
	}
	else if (shaderType == GL_FRAGMENT_SHADER) {
		result = new PixelShader(filePath, {}, shaderHandle);
	}
	else if (shaderType == GL_GEOMETRY_SHADER) {
		result = new GeometryShader(filePath, {}, shaderHandle);
	}
	else if (shaderType == GL_COMPUTE_SHADER) {
		result = new ComputeShader(filePath, {}, shaderHandle);
	}
	else {
		// throw shader::shader_unknown_type_exception(path_to_file);
	}

	for (auto& file : code.loadedFiles) {
		delete[] file.content;
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

ShaderProgram::ShaderProgram(VertexShader* vertexShader, GeometryShader* geometryShader, PixelShader* pixelShader, GLuint handle):
vertexShader(vertexShader),
geometryShader(geometryShader),
pixelShader(pixelShader),
handle(handle) {
	this->uniforms = UniformSpec(this);
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
	glUseProgram(this->program->GetHandle());

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