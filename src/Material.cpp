#include <Material.h>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

void Material::Bind() {
	glUseProgram(this->shader->GetHandle());

	auto& uniforms = this->shader->GetUniforms();

	for (unsigned int i = 0; i < uniforms.variables.size(); i++) {
		int offset = uniforms.offsets[i];
		switch (uniforms.variables[i].type) {
		case UniformType::Float1:
			glUniform1f(i, GetValue<float>(i));
			break;
		case UniformType::Float2:
			glUniform2fv(i, 1, &GetValue<glm::vec2>(i)[0]);
			break;
		case UniformType::Float3:
			glUniform3fv(i, 1, &GetValue<glm::vec3>(i)[0]);
			break;
		case UniformType::Float4:
			glUniform4fv(i, 1, &GetValue<glm::vec4>(i)[0]);
			break;
		case UniformType::Uint1:
			glUniform1ui(i, GetValue<unsigned int>(i));
			break;
		case UniformType::Uint2:
			glUniform2uiv(i, 1, &GetValue<glm::uvec2>(i)[0]);
			break;
		case UniformType::Uint3:
			glUniform3uiv(i, 1, &GetValue<glm::uvec3>(i)[0]);
			break;
		case UniformType::Matrix3x3:
			glUniformMatrix3fv(i, 1, false, &GetValue<glm::mat3>(i)[0][0]);
			break;
		case UniformType::Matrix4x4:
			glUniformMatrix4fv(i, 1, false, &GetValue<glm::mat4>(i)[0][0]);
			break;
		}
	}
}

Material::Material(ShaderProgram* shader):
shader(shader) {
	unsigned int uniformBufferSize = shader->GetUniforms().GetBufferSize();
	
	this->dataBuffer = (void*) new char[uniformBufferSize];
}

const ShaderProgram* Material::GetShader() const {
	return this->shader;
}
