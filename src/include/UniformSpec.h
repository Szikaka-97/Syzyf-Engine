#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <concepts>

#include <glm/fwd.hpp>

#include <Texture.h>

template <class T>
concept Blittable = ( // It's stupid, but frankly idk if there's a better way to do it
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
	Unsupported
};

struct UniformVariable {
	UniformType type;
	std::string name;
};

struct TextureVariable {
	TextureType type;
	std::string name;
};

class IncompatibleUniformTypeException : public std::runtime_error {
	template<Blittable T>
	IncompatibleUniformTypeException(const UniformVariable& variable);
};

struct UniformSpec {
	std::vector<UniformVariable> variables;
	std::vector<int> offsets;

	UniformSpec();
	UniformSpec(std::vector<UniformVariable> variables);

	unsigned int GetBufferSize() const;
};

