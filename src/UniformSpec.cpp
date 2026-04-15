#include <UniformSpec.h>

#include <glad/glad.h>
#include <malloc.h>

#include <Shader.h>

struct UniformTypeInfo {
	UniformSpec::UniformType type;
	int size;
};

UniformTypeInfo GetUniformInfo(GLenum type) {
	const static std::map<GLenum, UniformTypeInfo> dict {
		{ GL_FLOAT, { UniformSpec::UniformType::Float1, 1 * sizeof(GLfloat)} },
		{ GL_FLOAT_VEC2, { UniformSpec::UniformType::Float2, 2 * sizeof(GLfloat)} },
		{ GL_FLOAT_VEC3, { UniformSpec::UniformType::Float3, 3 * sizeof(GLfloat)} },
		{ GL_FLOAT_VEC4, { UniformSpec::UniformType::Float4, 4 * sizeof(GLfloat)} },
		{ GL_UNSIGNED_INT, { UniformSpec::UniformType::Uint1, 1 * sizeof(GLuint)} },
		{ GL_UNSIGNED_INT_VEC2, { UniformSpec::UniformType::Uint2, 2 * sizeof(GLuint)} },
		{ GL_UNSIGNED_INT_VEC3, { UniformSpec::UniformType::Uint3, 3 * sizeof(GLuint)} },
		{ GL_UNSIGNED_INT_VEC4, { UniformSpec::UniformType::Uint4, 4 * sizeof(GLuint)} },
		{ GL_FLOAT_MAT3, { UniformSpec::UniformType::Matrix3x3, 9 * sizeof(GLfloat)} },
		{ GL_FLOAT_MAT4, { UniformSpec::UniformType::Matrix4x4, 16 * sizeof(GLfloat)} },
		{ GL_SAMPLER_2D, { UniformSpec::UniformType::Sampler2D, sizeof(UniformSpec::TextureUniform<Texture2D>)} },
		{ GL_SAMPLER_CUBE, { UniformSpec::UniformType::Cubemap, sizeof(UniformSpec::TextureUniform<Cubemap>)} },
		{ GL_IMAGE_2D, { UniformSpec::UniformType::Image2D, sizeof(UniformSpec::TextureUniform<Texture2D>)} },
		{ GL_UNSIGNED_INT_IMAGE_2D, { UniformSpec::UniformType::UImage2D, sizeof(UniformSpec::TextureUniform<Texture2D>)} },
		{ GL_IMAGE_CUBE, { UniformSpec::UniformType::ImageCube, sizeof(UniformSpec::TextureUniform<Cubemap>)} },
	};

	if (dict.contains(type)) {
		return dict.at(type);
	}

	return { UniformSpec::UniformType::Unsupported, 0 };
}

void UniformSpec::CreateFrom(GLuint programHandle) {	
	int uniformCount = 0;

	glGetProgramiv(programHandle, GL_ACTIVE_UNIFORMS, &uniformCount);

	std::vector<GLuint> uniforms(uniformCount);
	std::vector<GLint> params(uniformCount);

	std::vector<GLuint> shaderUniforms;

	for (int i = 0; i < uniformCount; i++) {
		uniforms[i] = i;
	}

	glGetActiveUniformsiv(programHandle, uniformCount, uniforms.data(), GL_UNIFORM_BLOCK_INDEX, params.data());

	for (int i = 0; i < uniformCount; i++) {
		if (params[i] < 0) {
			shaderUniforms.push_back(i);
		}
	}

	uniformCount = shaderUniforms.size();

	this->variables.resize(uniformCount);

	glGetActiveUniformsiv(programHandle, uniformCount, shaderUniforms.data(), GL_UNIFORM_TYPE, params.data());

	int currentOffset = 0;

	for (int i = 0; i < uniformCount; i++) {
		this->variables[i].binding = shaderUniforms[i];
		
		auto typeInfo = GetUniformInfo(params[i]);

		this->variables[i].offset = currentOffset;
		this->variables[i].type = typeInfo.type;
		
		currentOffset += typeInfo.size;
	}

	glGetActiveUniformsiv(programHandle, uniformCount, shaderUniforms.data(), GL_UNIFORM_NAME_LENGTH, params.data());

	for (int i = 0; i < uniformCount; i++) {
		this->variables[i].name.resize(params[i] - 1);
		glGetActiveUniformName(programHandle, this->variables[i].binding, params[i], &params[i], this->variables[i].name.data());
		
		this->variables[i].binding = glGetUniformLocation(programHandle, this->variables[i].name.c_str());
	}

	this->variablesBufferLength = currentOffset;
}

UniformSpec::UniformSpec(const ShaderProgram* program):
UniformSpec() {
	GLuint handle = program->GetHandle();

	CreateFrom(handle);
}

UniformSpec::UniformSpec(const ComputeShaderProgram* program):
UniformSpec() {
	GLuint handle = program->GetHandle();

	CreateFrom(handle);
}

unsigned int UniformSpec::GetBufferSize() const {
	return this->variablesBufferLength;
}

unsigned int UniformSpec::VariableCount() const {
	return this->variables.size();
}

unsigned int UniformSpec::UniformBuffersCount() const {
	return this->uniformBuffers.size();
}

unsigned int UniformSpec::StorageBuffersCount() const {
	return this->storageBuffers.size();
}

const UniformSpec::UniformVariableSpec& UniformSpec::VariableAt(int index) const {
	return this->variables.at(index);
}

const UniformSpec::UniformBufferSpec& UniformSpec::UniformBufferAt(int index) const {
	return this->uniformBuffers.at(index);
}

const UniformSpec::ShaderStorageBufferSpec& UniformSpec::StorageBufferAt(int index) const {
	return this->storageBuffers.at(index);
}

const UniformSpec::UniformVariableSpec& UniformSpec::operator[](int index) const {
	return VariableAt(index);
}