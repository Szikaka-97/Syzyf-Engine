#include <UniformSpec.h>

#include <glad/glad.h>

unsigned int UniformTypeSizes[] {
	1 * sizeof(GLfloat),
	2 * sizeof(GLfloat),
	3 * sizeof(GLfloat),
	4 * sizeof(GLfloat),
	1 * sizeof(GLuint),
	2 * sizeof(GLuint),
	3 * sizeof(GLuint),
	4 * sizeof(GLuint),
	9 * sizeof(GLfloat),
	16 * sizeof(GLfloat),
	sizeof(Texture*),
	0
};

UniformSpec::UniformSpec() { }

UniformSpec::UniformSpec(std::vector<UniformVariable> variables):
variables(variables){
	int offset = 0;
	for (const auto& var : variables) {
		this->offsets.push_back(offset);

		offset += UniformTypeSizes[(int) var.type];
	}
}

unsigned int UniformSpec::GetBufferSize() const {
	unsigned int size = 0;

	for (const auto& var : this->variables) {
		size += UniformTypeSizes[(int) var.type];
	}

	return size;
}
