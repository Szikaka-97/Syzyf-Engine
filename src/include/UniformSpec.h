#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <concepts>

#include <glm/fwd.hpp>

#include <Texture.h>

class ShaderProgram;
class ComputeShaderProgram;

template <class T>
concept Blittable = ( // It's stupid, but frankly idk if there's a better way to do it
	std::same_as<T, bool> //idk
  ||
  std::same_as<T, float>
	||
	std::same_as<T, glm::vec2>
	||
	std::same_as<T, glm::vec3>
	||
	std::same_as<T, glm::vec4>
	||
	std::same_as<T, unsigned int>
	||
	std::same_as<T, glm::uvec2>
	||
	std::same_as<T, glm::uvec3>
	||
	std::same_as<T, glm::uvec4>
	||
	std::same_as<T, glm::mat3>
	||
	std::same_as<T, glm::mat4>
);

template <class T>
concept TextureClass = (
	std::derived_from<T, Texture>
);

class UniformSpec {
public:
	enum class UniformType {
		Float1,
		Float2,
		Float3,
		Float4,
		Uint1,
		Uint2,
		Uint3,
		Uint4,
		Matrix3x3,
		Matrix4x4,
		Sampler2D,
		Cubemap,
		Image2D,
		UImage2D,
		ImageCube,
		Unsupported
	};
	
	struct UniformVariableSpec {
		UniformType type;
		int offset;
		int binding;
		std::string name;
	};
	
	struct UniformBufferSpec {
		std::string name;
		int binding;
		int size;
	};
	
	struct ShaderStorageBufferSpec {
		std::string name;
		int binding;
		int stride;
	};

	template<TextureClass T>
	struct TextureUniform {
		T* tex;
		unsigned int level;
	};
private:
	std::vector<UniformVariableSpec> variables;
	int variablesBufferLength;
	std::vector<UniformBufferSpec> uniformBuffers;
	std::vector<ShaderStorageBufferSpec> storageBuffers;
	void CreateFrom(GLuint programHandle);
public:
	UniformSpec();
	UniformSpec(const ShaderProgram* program);
	UniformSpec(const ComputeShaderProgram* program);

	unsigned int GetBufferSize() const;
	unsigned int VariableCount() const;
	unsigned int UniformBuffersCount() const;
	unsigned int StorageBuffersCount() const;

	const UniformVariableSpec& VariableAt(int index) const;
	const UniformBufferSpec& UniformBufferAt(int index) const;
	const ShaderStorageBufferSpec& StorageBufferAt(int index) const;

	const UniformVariableSpec& operator[](int index) const;
};
