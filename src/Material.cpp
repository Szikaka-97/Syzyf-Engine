#include <Material.h>

void ShaderVariableStorage::Bind() const {
	int samplerIndex = 0;

	for (unsigned int i = 0; i < this->uniformSpec->VariableCount(); i++) {
		int offset = this->uniformSpec->VariableAt(i).offset;

		switch (this->uniformSpec->VariableAt(i).type) {
		case UniformSpec::UniformType::Float1:
			glUniform1f(this->uniformSpec->VariableAt(i).binding, GetValue<float>(i));
			break;
		case UniformSpec::UniformType::Float2:
			glUniform2fv(this->uniformSpec->VariableAt(i).binding, 1, &GetValue<glm::vec2>(i)[0]);
			break;
		case UniformSpec::UniformType::Float3:
			glUniform3fv(this->uniformSpec->VariableAt(i).binding, 1, &GetValue<glm::vec3>(i)[0]);
			break;
		case UniformSpec::UniformType::Float4:
			glUniform4fv(this->uniformSpec->VariableAt(i).binding, 1, &GetValue<glm::vec4>(i)[0]);
			break;
		case UniformSpec::UniformType::Uint1:
			glUniform1ui(this->uniformSpec->VariableAt(i).binding, GetValue<unsigned int>(i));
			break;
		case UniformSpec::UniformType::Uint2:
			glUniform2uiv(this->uniformSpec->VariableAt(i).binding, 1, &GetValue<glm::uvec2>(i)[0]);
			break;
		case UniformSpec::UniformType::Uint3:
			glUniform3uiv(this->uniformSpec->VariableAt(i).binding, 1, &GetValue<glm::uvec3>(i)[0]);
			break;
		case UniformSpec::UniformType::Matrix3x3:
			glUniformMatrix3fv(this->uniformSpec->VariableAt(i).binding, 1, false, &GetValue<glm::mat3>(i)[0][0]);
			break;
		case UniformSpec::UniformType::Matrix4x4:
			glUniformMatrix4fv(this->uniformSpec->VariableAt(i).binding, 1, false, &GetValue<glm::mat4>(i)[0][0]);
			break;
		case UniformSpec::UniformType::Sampler2D:
		{
			Texture2D* imageTex = GetValue<Texture2D>(i);

			GLuint imageTexHandle = 0;

			if (imageTex) {
				if (imageTex->IsDirty()) {
					imageTex->Update();
				}
				
				imageTexHandle = imageTex->GetHandle();
			}
			
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			glBindTexture(GL_TEXTURE_2D, imageTexHandle);
			glUniform1i(this->uniformSpec->VariableAt(i).binding, samplerIndex);

			samplerIndex++;

			break;
		}
		case UniformSpec::UniformType::Cubemap:
		{
			Cubemap* cubeTex = GetValue<Cubemap>(i);

			GLuint cubeTexHandle = 0;

			if (cubeTex) {
				if (cubeTex->IsDirty()) {
					cubeTex->Update();
				}
				
				cubeTexHandle = cubeTex->GetHandle();
			}
			
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexHandle);
			glUniform1i(this->uniformSpec->VariableAt(i).binding, samplerIndex);

			samplerIndex++;

			break;
		}
		case UniformSpec::UniformType::Image2D:
		case UniformSpec::UniformType::UImage2D:
		{
			Texture2D* imageTex = GetValue<Texture2D>(i);

			GLuint imageTexHandle = 0;
			GLenum imageFormat = GL_RGBA16F;

			if (imageTex) {
				if (imageTex->IsDirty()) {
					imageTex->Update();
				}
				
				imageTexHandle = imageTex->GetHandle();
				imageFormat = Texture::CalcInternalFormat({
					.channels = imageTex->GetChannels(),
					.colorSpace = imageTex->GetColorSpace(),
					.format = imageTex->GetFormat(),
				});

				glUniform1i(this->uniformSpec->VariableAt(i).binding, samplerIndex);
			}

			glBindImageTexture(samplerIndex, imageTexHandle, 0, false, 0, GL_READ_WRITE, imageFormat);

			samplerIndex++;

			break;
		}
		}
	}

	for (unsigned int i = 0; i < this->uniformSpec->UniformBuffersCount(); i++) {
		auto uniformBufferSpec = this->uniformSpec->UniformBufferAt(i);
		auto uniformBufferData = uniformBuffers[i];

		glBindBuffer(GL_UNIFORM_BUFFER, uniformBufferData.bufferHandle);
		glBufferData(GL_UNIFORM_BUFFER, uniformBufferSpec.size, uniformBufferData.bufferData, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glBindBufferBase(GL_UNIFORM_BUFFER, uniformBufferSpec.binding, uniformBufferData.bufferHandle);
	}

	for (unsigned int i = 0; i < this->uniformSpec->StorageBuffersCount(); i++) {
		auto storageBufferData = storageBuffers[i];

		// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, storageBufferData.bufferHandle);
	}
}

ShaderVariableStorage::ShaderVariableStorage(const UniformSpec& uniformSpec):
uniformSpec(&uniformSpec) {
	unsigned int variableBufferSize = uniformSpec.GetBufferSize();
	
	this->dataBuffer = (void*) new char[variableBufferSize];
	memset(this->dataBuffer, 0, variableBufferSize);

	int uniformBuffersCount = uniformSpec.UniformBuffersCount();

	this->uniformBuffers = new BufferPair[uniformBuffersCount];
	GLuint uniformBufferHandles[uniformBuffersCount];

	glGenBuffers(uniformBuffersCount, uniformBufferHandles);

	for (int i = 0; i < uniformBuffersCount; i++) {
		GLuint bufferHandle = uniformBufferHandles[i];
		unsigned int bufferSize = uniformSpec.UniformBufferAt(i).size;

		this->uniformBuffers[i].bufferData = (void*) new std::byte[bufferSize];
		memset(this->uniformBuffers[i].bufferData, 0, bufferSize);

		this->uniformBuffers[i].bufferHandle = bufferHandle;
	}

	int storageBuffersCount = uniformSpec.StorageBuffersCount();

	this->storageBuffers = new BufferPair[storageBuffersCount];
	memset(this->storageBuffers, 0, storageBuffersCount * sizeof(BufferPair));
}

const UniformSpec* ShaderVariableStorage::GetUniforms() const {
	return this->uniformSpec;
}

GLuint ShaderVariableStorage::GetStorageBuffer(const std::string& storageBufferName) {
	UniformSpec spec = *this->uniformSpec;

	for (int i = 0; i < spec.StorageBuffersCount(); i++) {
		if (spec.StorageBufferAt(i).name == storageBufferName) {
			return GetStorageBuffer(i);
		}
	}

	return 0;
}

GLuint ShaderVariableStorage::GetStorageBuffer(int storageBufferIndex) {
	if (storageBufferIndex < 0 || storageBufferIndex >= this->uniformSpec->StorageBuffersCount()) {
		return 0;
	}

	return this->storageBuffers[storageBufferIndex].bufferHandle;
}

void ShaderVariableStorage::BindStorageBuffer(const std::string& storageBufferName, GLuint bufferHandle) {
	UniformSpec spec = *this->uniformSpec;

	for (int i = 0; i < spec.StorageBuffersCount(); i++) {
		if (spec.StorageBufferAt(i).name == storageBufferName) {
			BindStorageBuffer(i, bufferHandle);

			return;
		}
	}
}

void ShaderVariableStorage::BindStorageBuffer(int storageBufferIndex, GLuint bufferHandle) {
	if (storageBufferIndex < 0 || storageBufferIndex >= this->uniformSpec->StorageBuffersCount()) {
		return;
	}

	this->storageBuffers[storageBufferIndex].bufferHandle = bufferHandle;
}

Material::Material(const ShaderProgram* shader):
shader(shader),
shaderVariables(shader->GetUniforms()) { }

void Material::Bind() const {
	glUseProgram(this->shader->GetHandle());

	this->shaderVariables.Bind();
}

GLuint Material::GetStorageBuffer(const std::string& storageBufferName) {
	return this->shaderVariables.GetStorageBuffer(storageBufferName);
}
GLuint Material::GetStorageBuffer(int storageBufferIndex) {
	return this->shaderVariables.GetStorageBuffer(storageBufferIndex);
}

void Material::BindStorageBuffer(const std::string& storageBufferName, GLuint bufferHandle) {
	this->shaderVariables.BindStorageBuffer(storageBufferName, bufferHandle);
}
void Material::BindStorageBuffer(int storageBufferIndex, GLuint bufferHandle) {
	this->shaderVariables.BindStorageBuffer(storageBufferIndex, bufferHandle);
}

const ShaderProgram* Material::GetShader() const {
	return this->shader;
}
const UniformSpec* Material::GetUniforms() const {
	return this->shaderVariables.GetUniforms();
}

ComputeDispatchData::ComputeDispatchData(const ComputeShaderProgram* shader):
shader(shader),
shaderVariables(shader->GetUniforms()) { }

void ComputeDispatchData::Bind() const {
	glUseProgram(this->shader->GetHandle());

	this->shaderVariables.Bind();
}

GLuint ComputeDispatchData::GetStorageBuffer(const std::string& storageBufferName) {
	return this->shaderVariables.GetStorageBuffer(storageBufferName);
}
GLuint ComputeDispatchData::GetStorageBuffer(int storageBufferIndex) {
	return this->shaderVariables.GetStorageBuffer(storageBufferIndex);
}

void ComputeDispatchData::BindStorageBuffer(const std::string& storageBufferName, GLuint bufferHandle) {
	this->shaderVariables.BindStorageBuffer(storageBufferName, bufferHandle);
}
void ComputeDispatchData::BindStorageBuffer(int storageBufferIndex, GLuint bufferHandle) {
	this->shaderVariables.BindStorageBuffer(storageBufferIndex, bufferHandle);
}

const ComputeShaderProgram* ComputeDispatchData::GetShader() const {
	return this->shader;
}
const UniformSpec* ComputeDispatchData::GetUniforms() const {
	return this->shaderVariables.GetUniforms();
}