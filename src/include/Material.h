#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <UniformSpec.h>
#include <Shader.h>
#include <Texture.h>

class ShaderVariableStorage {
	friend class SceneGraphics;
private:
	struct BufferPair {
		void* bufferData;
		GLuint bufferHandle;
	};

	void* dataBuffer;
	BufferPair* uniformBuffers;
	BufferPair* storageBuffers;
	const UniformSpec* uniformSpec;
	bool dirty;
public:
	ShaderVariableStorage(const UniformSpec& uniformSpec);
	
	void Bind() const;

	template<Blittable T>
	T GetValue(const std::string& uniformName) const;
	template<Blittable T>
	T GetValue(unsigned int uniformIndex) const;

	template<TextureClass T>
	UniformSpec::TextureUniform<T> GetValue(const std::string& uniformName) const;
	template<TextureClass T>
	UniformSpec::TextureUniform<T> GetValue(unsigned int uniformIndex) const;
	
	template<Blittable T>
	void SetValue(const std::string& uniformName, const T& value);
	template<Blittable T>
	void SetValue(unsigned int uniformIndex, const T& value);

	template<TextureClass T>
	void SetValue(const std::string& uniformName, T* value, unsigned int level = 0);
	template<TextureClass T>
	void SetValue(unsigned int uniformIndex, T* value, unsigned int level = 0);

	template<typename T_BufferRep>
	T_BufferRep GetUniformBuffer(const std::string& uniformBufferName);
	template<typename T_BufferRep>
	T_BufferRep GetUniformBuffer(int uniformBufferBinding);

	template<typename T_BufferRep>
	void SetUniformBuffer(const std::string& uniformBufferName, const T_BufferRep* data);
	template<typename T_BufferRep>
	void SetUniformBuffer(int uniformBufferBinding, const T_BufferRep* data);

	GLuint GetStorageBuffer(const std::string& storageBufferName);
	GLuint GetStorageBuffer(int storageBufferIndex);

	void BindStorageBuffer(const std::string& storageBufferName, GLuint bufferHandle);
	void BindStorageBuffer(int storageBufferIndex, GLuint bufferHandle);

	const UniformSpec* GetUniforms() const;
};

class Material {
private:
	const ShaderProgram* shader;
	ShaderVariableStorage shaderVariables;
public:
  std::string name;
	Material(const ShaderProgram* shader);

	void Bind() const;

	template<Blittable T>
	T GetValue(const std::string& uniformName) const;
	template<Blittable T>
	T GetValue(unsigned int uniformIndex) const;

	template<TextureClass T>
	T* GetValue(const std::string& uniformName) const;
	template<TextureClass T>
	T* GetValue(unsigned int uniformIndex) const;
	
	template<Blittable T>
	void SetValue(const std::string& uniformName, const T& value);
	template<Blittable T>
	void SetValue(unsigned int uniformIndex, const T& value);

	template<TextureClass T>
	void SetValue(const std::string& uniformName, T* value, unsigned int level = 0);
	template<TextureClass T>
	void SetValue(unsigned int uniformIndex, T* value, unsigned int level = 0);

	template<typename T_BufferRep>
	T_BufferRep GetUniformBuffer(const std::string& uniformBufferName);
	template<typename T_BufferRep>
	T_BufferRep GetUniformBuffer(int uniformBufferBinding);

	template<typename T_BufferRep>
	void SetUniformBuffer(const std::string& uniformBufferName, const T_BufferRep* data);
	template<typename T_BufferRep>
	void SetUniformBuffer(int uniformBufferBinding, const T_BufferRep* data);

	GLuint GetStorageBuffer(const std::string& storageBufferName);
	GLuint GetStorageBuffer(int storageBufferIndex);

	void BindStorageBuffer(const std::string& storageBufferName, GLuint bufferHandle);
	void BindStorageBuffer(int storageBufferIndex, GLuint bufferHandle);

	const ShaderProgram* GetShader() const;
	const UniformSpec* GetUniforms() const;
};

class ComputeDispatchData {
private:
	const ComputeShaderProgram* shader;
	ShaderVariableStorage shaderVariables;
public:
	ComputeDispatchData(const ComputeShaderProgram* shader);

	void Bind() const;

	template<Blittable T>
	T GetValue(const std::string& uniformName) const;
	template<Blittable T>
	T GetValue(unsigned int uniformIndex) const;

	template<TextureClass T>
	T* GetValue(const std::string& uniformName) const;
	template<TextureClass T>
	T* GetValue(unsigned int uniformIndex) const;
	
	template<Blittable T>
	void SetValue(const std::string& uniformName, const T& value);
	template<Blittable T>
	void SetValue(unsigned int uniformIndex, const T& value);

	template<TextureClass T>
	void SetValue(const std::string& uniformName, T* value, unsigned int level = 0);
	template<TextureClass T>
	void SetValue(unsigned int uniformIndex, T* value, unsigned int level = 0);

	template<typename T_BufferRep>
	T_BufferRep GetUniformBuffer(const std::string& uniformBufferName);
	template<typename T_BufferRep>
	T_BufferRep GetUniformBuffer(int uniformBufferBinding);

	template<typename T_BufferRep>
	void SetUniformBuffer(const std::string& uniformBufferName, const T_BufferRep* data);
	template<typename T_BufferRep>
	void SetUniformBuffer(int uniformBufferBinding, const T_BufferRep* data);

	GLuint GetStorageBuffer(const std::string& storageBufferName);
	GLuint GetStorageBuffer(int storageBufferIndex);

	void BindStorageBuffer(const std::string& storageBufferName, GLuint bufferHandle);
	void BindStorageBuffer(int storageBufferIndex, GLuint bufferHandle);

	const ComputeShaderProgram* GetShader() const;
	const UniformSpec* GetUniforms() const;
};

template<Blittable T>
static inline bool IsUniformOfRightType(UniformSpec::UniformSpec::UniformType type) {
	return false;
}

template<TextureClass T>
static inline bool IsUniformOfRightType(UniformSpec::UniformSpec::UniformType type) {
	return false;
}

#pragma region ShaderVariableStorage implementation

template<Blittable T>
T ShaderVariableStorage::GetValue(const std::string& uniformName) const {
	for (int i = 0; i < this->uniformSpec->VariableCount(); i++) {
		if (this->uniformSpec->VariableAt(i).name == uniformName) {
			return GetValue<T>(i);
		}
	}

	return T{};
}

template<TextureClass T>
UniformSpec::TextureUniform<T> ShaderVariableStorage::GetValue(const std::string& uniformName) const {
	for (int i = 0; i < this->uniformSpec->VariableCount(); i++) {
		if (this->uniformSpec->VariableAt(i).name == uniformName) {
			return GetValue<T>(i);
		}
	}

	return UniformSpec::TextureUniform<T>{nullptr, 0};
}

template<Blittable T>
T ShaderVariableStorage::GetValue(unsigned int uniformIndex) const {
	if (uniformIndex < 0 || uniformIndex >= this->uniformSpec->VariableCount()) {
		return T{};
	}

	if (!IsUniformOfRightType<T>(this->uniformSpec->VariableAt(uniformIndex).type)) {
		return T{};
	}

	return *((T*) ((char*) this->dataBuffer + this->uniformSpec->VariableAt(uniformIndex).offset));
}

template<TextureClass T>
UniformSpec::TextureUniform<T> ShaderVariableStorage::GetValue(unsigned int uniformIndex) const {
	if (uniformIndex < 0 || uniformIndex >= this->uniformSpec->VariableCount()) {
		return UniformSpec::TextureUniform<T>{nullptr, 0};
	}

	if (!IsUniformOfRightType<T>(this->uniformSpec->VariableAt(uniformIndex).type)) {
		return UniformSpec::TextureUniform<T>{nullptr, 0};
	}

	return *((UniformSpec::TextureUniform<T>*) ((char*) this->dataBuffer + this->uniformSpec->VariableAt(uniformIndex).offset));
}

template<Blittable T>
void ShaderVariableStorage::SetValue(const std::string& uniformName, const T& value) {
	for (int i = 0; i < this->uniformSpec->VariableCount(); i++) {
		if (this->uniformSpec->VariableAt(i).name == uniformName) {
			SetValue<T>(i, value);
		}
	}
}

template<TextureClass T>
void ShaderVariableStorage::SetValue(const std::string& uniformName, T* value, unsigned int level) {
	for (int i = 0; i < this->uniformSpec->VariableCount(); i++) {
		if (this->uniformSpec->VariableAt(i).name == uniformName) {
			SetValue<T>(i, value, level);
		}
	}
}
template<Blittable T>
void ShaderVariableStorage::SetValue(unsigned int uniformIndex, const T& value) {
	if (uniformIndex < 0 || uniformIndex >= this->uniformSpec->VariableCount()) {
		return;
	}

	if (!IsUniformOfRightType<T>(this->uniformSpec->VariableAt(uniformIndex).type)) {
		return;
	}

	*((T*) ((char*) this->dataBuffer + this->uniformSpec->VariableAt(uniformIndex).offset)) = value;
}

template<TextureClass T>
void ShaderVariableStorage::SetValue(unsigned int uniformIndex, T* value, unsigned int level) {
	if (uniformIndex < 0 || uniformIndex >= this->uniformSpec->VariableCount()) {
		return;
	}

	if (!IsUniformOfRightType<T>(this->uniformSpec->VariableAt(uniformIndex).type)) {
		return;
	}

	*((UniformSpec::TextureUniform<T>*) ((char*) this->dataBuffer + this->uniformSpec->VariableAt(uniformIndex).offset)) = UniformSpec::TextureUniform<T>{value, level};
}

template<typename T_BufferRep>
T_BufferRep ShaderVariableStorage::GetUniformBuffer(const std::string& uniformBufferName) {
	UniformSpec spec = *this->uniformSpec;

	for (int i = 0; i < spec.UniformBuffersCount(); i++) {
		if (spec.UniformBufferAt(i).name == uniformBufferName) {
			return GetUniformBuffer<T_BufferRep>(spec.UniformBufferAt(i).binding);
		}
	}

	return T_BufferRep{};
}

template<typename T_BufferRep>
T_BufferRep ShaderVariableStorage::GetUniformBuffer(int uniformBufferBinding) {
	if (uniformBufferBinding < 0) {
		return T_BufferRep{};
	}

	int bufferIndexInStorage = -1;

	for (int i = 0; i < this->uniformSpec->UniformBuffersCount(); i++) {
		if (this->uniformSpec->UniformBufferAt(i).binding == uniformBufferBinding) {
			bufferIndexInStorage = i;
			break;
		}
	}

	T_BufferRep result;
	
	if (bufferIndexInStorage >= 0) {
		void* storageBuffer = this->uniformBuffers[bufferIndexInStorage].bufferData;
		UniformSpec::UniformBufferSpec bufferSpec = this->uniformSpec->UniformBufferAt(bufferIndexInStorage);
	
		memcpy(&result, storageBuffer, sizeof(T_BufferRep) < bufferSpec.size ? sizeof(T_BufferRep) : bufferSpec.size);
	}

	return result;
}

template<typename T_BufferRep>
void ShaderVariableStorage::SetUniformBuffer(const std::string& uniformBufferName, const T_BufferRep* data) {
	UniformSpec spec = *this->uniformSpec;

	for (int i = 0; i < spec.UniformBuffersCount(); i++) {
		if (spec.UniformBufferAt(i).name == uniformBufferName) {
			SetUniformBuffer<T_BufferRep>(spec.UniformBufferAt(i).binding, data);

			return;
		}
	}
}

template<typename T_BufferRep>
void ShaderVariableStorage::SetUniformBuffer(int uniformBufferBinding, const T_BufferRep* data) {
	if (uniformBufferBinding < 0) {
		return;
	}

	int bufferIndexInStorage = -1;

	for (int i = 0; i < this->uniformSpec->UniformBuffersCount(); i++) {
		if (this->uniformSpec->UniformBufferAt(i).binding == uniformBufferBinding) {
			bufferIndexInStorage = i;
			break;
		}
	}

	if (bufferIndexInStorage < 0) {
		return;
	}

	void* storageBuffer = this->uniformBuffers[bufferIndexInStorage].bufferData;
	UniformSpec::UniformBufferSpec bufferSpec = this->uniformSpec->UniformBufferAt(bufferIndexInStorage);

	memcpy(storageBuffer, data, sizeof(T_BufferRep) < bufferSpec.size ? sizeof(T_BufferRep) : bufferSpec.size);
}

#pragma endregion

#pragma region Material implementation

template<Blittable T>
T Material::GetValue(const std::string& uniformName) const {
	return this->shaderVariables.GetValue<T>(uniformName);
}
template<Blittable T>
T Material::GetValue(unsigned int uniformIndex) const {
	return this->shaderVariables.GetValue<T>(uniformIndex);
}

template<TextureClass T>
T* Material::GetValue(const std::string& uniformName) const {
	return this->shaderVariables.GetValue<T>(uniformName).tex;
}
template<TextureClass T>
T* Material::GetValue(unsigned int uniformIndex) const {
	return this->shaderVariables.GetValue<T>(uniformIndex).tex;
}

template<Blittable T>
void Material::SetValue(const std::string& uniformName, const T& value) {
	this->shaderVariables.SetValue(uniformName, value);
}
template<Blittable T>
void Material::SetValue(unsigned int uniformIndex, const T& value) {
	this->shaderVariables.SetValue(uniformIndex, value);
}

template<TextureClass T>
void Material::SetValue(const std::string& uniformName, T* value, unsigned int level) {
	this->shaderVariables.SetValue(uniformName, value, level);
}
template<TextureClass T>
void Material::SetValue(unsigned int uniformIndex, T* value, unsigned int level) {
	this->shaderVariables.SetValue(uniformIndex, value, level);
}

template<typename T_BufferRep>
T_BufferRep Material::GetUniformBuffer(const std::string& uniformBufferName) {
	return this->shaderVariables.GetUniformBuffer<T_BufferRep>(uniformBufferName);
}
template<typename T_BufferRep>
T_BufferRep Material::GetUniformBuffer(int uniformBufferBinding) {
	return this->shaderVariables.GetUniformBuffer<T_BufferRep>(uniformBufferBinding);
}

template<typename T_BufferRep>
void Material::SetUniformBuffer(const std::string& uniformBufferName, const T_BufferRep* data) {
	this->shaderVariables.SetUniformBuffer(uniformBufferName, data);
}
template<typename T_BufferRep>
void Material::SetUniformBuffer(int uniformBufferBinding, const T_BufferRep* data) {
	this->shaderVariables.SetUniformBuffer(uniformBufferBinding, data);
}

#pragma endregion

#pragma region ComputeDispatchData implementation

template<Blittable T>
T ComputeDispatchData::GetValue(const std::string& uniformName) const {
	return this->shaderVariables.GetValue<T>(uniformName);
}
template<Blittable T>
T ComputeDispatchData::GetValue(unsigned int uniformIndex) const {
	return this->shaderVariables.GetValue<T>(uniformIndex);
}

template<TextureClass T>
T* ComputeDispatchData::GetValue(const std::string& uniformName) const {
	return this->shaderVariables.GetValue<T>(uniformName).tex;
}
template<TextureClass T>
T* ComputeDispatchData::GetValue(unsigned int uniformIndex) const {
	return this->shaderVariables.GetValue<T>(uniformIndex).tex;
}
template<Blittable T>
void ComputeDispatchData::SetValue(const std::string& uniformName, const T& value) {
	this->shaderVariables.SetValue(uniformName, value);
}
template<Blittable T>
void ComputeDispatchData::SetValue(unsigned int uniformIndex, const T& value) {
	this->shaderVariables.SetValue(uniformIndex, value);
}

template<TextureClass T>
void ComputeDispatchData::SetValue(const std::string& uniformName, T* value, unsigned int level) {
	this->shaderVariables.SetValue(uniformName, value, level);
}
template<TextureClass T>
void ComputeDispatchData::SetValue(unsigned int uniformIndex, T* value, unsigned int level) {
	this->shaderVariables.SetValue(uniformIndex, value, level);
}

template<typename T_BufferRep>
T_BufferRep ComputeDispatchData::GetUniformBuffer(const std::string& uniformBufferName) {
	return this->shaderVariables.GetUniformBuffer<T_BufferRep>(uniformBufferName);
}
template<typename T_BufferRep>
T_BufferRep ComputeDispatchData::GetUniformBuffer(int uniformBufferBinding) {
	return this->shaderVariables.GetUniformBuffer<T_BufferRep>(uniformBufferBinding);
}

template<typename T_BufferRep>
void ComputeDispatchData::SetUniformBuffer(const std::string& uniformBufferName, const T_BufferRep* data) {
	this->shaderVariables.SetUniformBuffer(uniformBufferName, data);
}
template<typename T_BufferRep>
void ComputeDispatchData::SetUniformBuffer(int uniformBufferBinding, const T_BufferRep* data) {
	this->shaderVariables.SetUniformBuffer(uniformBufferBinding, data);
}

#pragma endregion

template<> inline bool IsUniformOfRightType<float>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Float1;
}
template<> inline bool IsUniformOfRightType<glm::vec2>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Float2;
}
template<> inline bool IsUniformOfRightType<glm::vec3>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Float3;
}
template<> inline bool IsUniformOfRightType<glm::vec4>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Float4;
}
template<> inline bool IsUniformOfRightType<unsigned int>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Uint1;
}
template<> inline bool IsUniformOfRightType<glm::uvec2>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Uint2;
}
template<> inline bool IsUniformOfRightType<glm::uvec3>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Uint3;
}
template<> inline bool IsUniformOfRightType<glm::uvec4>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Uint4;
}
template<> inline bool IsUniformOfRightType<glm::mat3>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Matrix3x3;
}
template<> inline bool IsUniformOfRightType<glm::mat4>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Matrix4x4;
}
template<> inline bool IsUniformOfRightType<Texture2D>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Sampler2D || type == UniformSpec::UniformType::Image2D || type == UniformSpec::UniformType::UImage2D;
}
template<> inline bool IsUniformOfRightType<Cubemap>(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Cubemap || type == UniformSpec::UniformType::ImageCube;
}
