#pragma once

#include <UniformSpec.h>
#include <Shader.h>

class Material {
	friend class SceneGraphics;
private:
	void* dataBuffer;
	ShaderProgram* shader;
	bool dirty;

	void Bind();

	template<Blittable T>
	static inline bool IsUniformOfRightType(UniformType type);
public:
	Material(ShaderProgram* shader);

	template<Blittable T>
	T GetValue(const std::string& uniformName) const;
	template<Blittable T>
	T GetValue(unsigned int uniformIndex) const;
	
	template<Blittable T>
	void SetValue(const std::string& uniformName, const T& value);
	template<Blittable T>
	void SetValue(unsigned int uniformIndex, const T& value);

	const ShaderProgram* GetShader() const;
};

template<Blittable T>
T Material::GetValue(const std::string& uniformName) const {
	return GetValue<T>(glGetUniformLocation(this->shader->GetHandle(), uniformName.c_str()));
}

template<Blittable T>
T Material::GetValue(unsigned int uniformIndex) const {
	if (uniformIndex < 0 || uniformIndex >= this->shader->GetUniforms().variables.size()) {
		return T{};
	}

	if (!IsUniformOfRightType<T>(this->shader->GetUniforms().variables[uniformIndex].type)) {
		return T{};
	}

	return *((T*) ((char*) this->dataBuffer + this->shader->GetUniforms().offsets[uniformIndex]));
}

template<Blittable T>
void Material::SetValue(const std::string& uniformName, const T& value) {
	SetValue<T>(glGetUniformLocation(this->shader->GetHandle(), uniformName.c_str()), value);
}

template<Blittable T>
void Material::SetValue(unsigned int uniformIndex, const T& value) {
	if (uniformIndex < 0 || uniformIndex >= this->shader->GetUniforms().variables.size()) {
		return;
	}

	if (!IsUniformOfRightType<T>(this->shader->GetUniforms().variables[uniformIndex].type)) {
		return;
	}

	*((T*) ((char*) this->dataBuffer + this->shader->GetUniforms().offsets[uniformIndex])) = value;
}

template<Blittable T>
inline bool Material::IsUniformOfRightType(UniformType type) {
	return false;
}

template<> inline bool Material::IsUniformOfRightType<float>(UniformType type) {
	return type == UniformType::Float1;
}
template<> inline bool Material::IsUniformOfRightType<glm::vec2>(UniformType type) {
	return type == UniformType::Float2;
}
template<> inline bool Material::IsUniformOfRightType<glm::vec3>(UniformType type) {
	return type == UniformType::Float3;
}
template<> inline bool Material::IsUniformOfRightType<glm::vec4>(UniformType type) {
	return type == UniformType::Float4;
}
template<> inline bool Material::IsUniformOfRightType<unsigned int>(UniformType type) {
	return type == UniformType::Uint1;
}
template<> inline bool Material::IsUniformOfRightType<glm::uvec2>(UniformType type) {
	return type == UniformType::Uint2;
}
template<> inline bool Material::IsUniformOfRightType<glm::uvec3>(UniformType type) {
	return type == UniformType::Uint3;
}
template<> inline bool Material::IsUniformOfRightType<glm::uvec4>(UniformType type) {
	return type == UniformType::Uint4;
}
template<> inline bool Material::IsUniformOfRightType<glm::mat3>(UniformType type) {
	return type == UniformType::Matrix3x3;
}
template<> inline bool Material::IsUniformOfRightType<glm::mat4>(UniformType type) {
	return type == UniformType::Matrix4x4;
}
