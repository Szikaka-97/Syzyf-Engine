#include <UniformSpec.h>

#include <glad/glad.h>
#include <cmath>

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
		{ GL_SAMPLER_2D, { UniformSpec::UniformType::Sampler2D, sizeof(Texture2D*)} },
		{ GL_SAMPLER_CUBE, { UniformSpec::UniformType::Cubemap, sizeof(Cubemap*)} },
		{ GL_IMAGE_2D, { UniformSpec::UniformType::Image2D, sizeof(Texture2D*)} },
		{ GL_UNSIGNED_INT_IMAGE_2D, { UniformSpec::UniformType::UImage2D, sizeof(Texture2D*)} },
	};

	if (dict.contains(type)) {
		return dict.at(type);
	}

	return { UniformSpec::UniformType::Unsupported, 0 };
}

bool IsTextureType(UniformSpec::UniformType type) {
	return type == UniformSpec::UniformType::Sampler2D || type == UniformSpec::UniformType::Cubemap || type == UniformSpec::UniformType::Image2D || type == UniformSpec::UniformType::UImage2D;
}

void UniformSpec::CreateFrom(GLuint programHandle) {
	int uniformBufferCount = 0;
	int uniformVariablesCount = 0;
	int storageBufferCount = 0;
	glGetProgramInterfaceiv(programHandle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniformBufferCount);
	glGetProgramInterfaceiv(programHandle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniformVariablesCount);
	glGetProgramInterfaceiv(programHandle, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &storageBufferCount);

	int uniformNameBufferSize = 0;
	glGetProgramiv(programHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameBufferSize);
	char uniformName[uniformNameBufferSize];

	// spdlog::info("┬─Uniform buffer count = {}", uniformBufferCount);
	// spdlog::info("├─Uniform variable count = {}", uniformVariablesCount);
	// spdlog::info("├─Storage buffer count = {}", storageBufferCount);

	bool freeUniforms[uniformVariablesCount];
	for (int i = 0; i < uniformVariablesCount; i++) {
		freeUniforms[i] = true;
	}

//	spdlog::info("┬┬─Uniform buffers [{}]:", uniformBufferCount);
	for (int uniformBufferIndex = 0; uniformBufferIndex < uniformBufferCount; uniformBufferIndex++) {
		struct {
			int nameLength;
			int bufferSize;
			int computeBuffer;
			int binding;
			int variablesCount;
		} propValues;

		GLenum bufferProps[] {
			GL_NAME_LENGTH,
			GL_BUFFER_DATA_SIZE,
			GL_REFERENCED_BY_COMPUTE_SHADER,
			GL_BUFFER_BINDING,
			GL_NUM_ACTIVE_VARIABLES
		};

		glGetProgramResourceiv(programHandle, GL_UNIFORM_BLOCK, uniformBufferIndex, sizeof(bufferProps) / sizeof(GLenum), bufferProps, sizeof(bufferProps) / sizeof(GLenum), nullptr, (int*) &propValues);
		
		char nameBuf[propValues.nameLength];

		glGetProgramResourceName(programHandle, GL_UNIFORM_BLOCK, uniformBufferIndex, propValues.nameLength, nullptr, nameBuf);

		bool isLast = uniformBufferIndex == uniformBufferCount - 1;

//		spdlog::info("│{}┬─Buffer no {} : {}", isLast ? "└" : "├", uniformBufferIndex, std::string(nameBuf));
//		spdlog::info("│{}├──Size = {}", isLast ? " " : "│", propValues.bufferSize);
//		spdlog::info("│{}├──In compute shader = {}", isLast ? " " : "│", (bool) propValues.computeBuffer);
//		spdlog::info("│{}├──Binding = {}", isLast ? " " : "│", propValues.binding);
//		spdlog::info("│{}└┬─Variables count = {}", isLast ? " " : "│", propValues.variablesCount);

		GLint bufferVariables[propValues.variablesCount];
		GLenum variableProp = GL_ACTIVE_VARIABLES;

		glGetProgramResourceiv(programHandle, GL_UNIFORM_BLOCK, uniformBufferIndex, 1, &variableProp, sizeof(bufferVariables) / sizeof(GLint), nullptr, bufferVariables);

		for (int bufferVariableIndex = 0; bufferVariableIndex < propValues.variablesCount; bufferVariableIndex++) {
			GLint bufferVariable = bufferVariables[bufferVariableIndex];

			freeUniforms[bufferVariable] = false;

			int uniformSize = 0;
			GLenum uniformType;
			glGetActiveUniform(programHandle, bufferVariable, uniformNameBufferSize, nullptr, &uniformSize, &uniformType, uniformName);

			bool isLastVariable = bufferVariableIndex == propValues.variablesCount - 1;

//			spdlog::info("│{} {}┬─Uniform variable no {} : {}", isLast ? " " : "│", isLastVariable ? "└" : "├", bufferVariableIndex, std::string(uniformName));
//			spdlog::info("│{} {}├──Type: {:x}", isLast ? " " : "│", isLastVariable ? " " : "│", uniformType);
//			spdlog::info("│{} {}└──Size: {:x}", isLast ? " " : "│", isLastVariable ? " " : "│", uniformSize);
		}

		if (propValues.computeBuffer || uniformBufferIndex >= 2) {
			this->uniformBuffers.push_back({ std::string(nameBuf), propValues.binding, propValues.bufferSize });
		}
	}

	this->variablesBufferLength = 0;
//	spdlog::info("├┬─Uniform variables [{}]:", uniformVariablesCount);
	for (int uniformVariableIndex = 0; uniformVariableIndex < uniformVariablesCount; uniformVariableIndex++) {
		if (!freeUniforms[uniformVariableIndex]) {
			continue;
		}

		int uniformSize = 0;
		GLenum uniformType;
		glGetActiveUniform(programHandle, uniformVariableIndex, uniformNameBufferSize, nullptr, &uniformSize, &uniformType, uniformName);

		bool isLast = uniformVariableIndex == uniformVariablesCount - 1;

//		spdlog::info("│{}┬─Uniform variable no {} : {}", isLast ? "└" : "├", uniformVariableIndex, std::string(uniformName));
//		spdlog::info("│{}├──Type: {:x}", isLast ? " " : "│", uniformType);
//		spdlog::info("│{}├──Bind: {}", isLast ? " " : "│", glGetUniformLocation(programHandle, uniformName));
//		spdlog::info("│{}└──Size: {}", isLast ? " " : "│", uniformSize);

		UniformTypeInfo info = GetUniformInfo(uniformType);

		this->variables.push_back({ info.type, this->variablesBufferLength, glGetUniformLocation(programHandle, uniformName), uniformName });
	
		this->variablesBufferLength += info.size;
	}

//	spdlog::info("└{}─Storage buffers [{}]:", storageBufferCount ? "┬" : "─", storageBufferCount);
	for (int storageBufferIndex = 0; storageBufferIndex < storageBufferCount; storageBufferIndex++) {
		struct {
			int nameLength;
			int bufferVariableCount;
			int binding;
		} propValues;

		GLenum bufferProps[] {
			GL_NAME_LENGTH,
			GL_NUM_ACTIVE_VARIABLES,
			GL_BUFFER_BINDING,
		};

		glGetProgramResourceiv(programHandle, GL_SHADER_STORAGE_BLOCK, storageBufferIndex, 3, bufferProps, 3, nullptr, (int*) &propValues);

		char nameBuf[propValues.nameLength];
		glGetProgramResourceName(programHandle, GL_SHADER_STORAGE_BLOCK, storageBufferIndex, propValues.nameLength, nullptr, nameBuf);

		bool isLast = storageBufferIndex == storageBufferCount - 1;

//		spdlog::info(" {}┬─Buffer no {} : {}", isLast ? "└" : "├", storageBufferIndex, std::string(nameBuf));

		GLint bufferVars[propValues.bufferVariableCount];
		GLenum variableProp = GL_ACTIVE_VARIABLES;

		glGetProgramResourceiv(programHandle, GL_SHADER_STORAGE_BLOCK, storageBufferIndex, 1, &variableProp, sizeof(bufferVars) / sizeof(GLint), nullptr, bufferVars);

		struct {
			int nameLength;
			int offset;
			int arraySize;
			int arrayStride;
			int topLevelArrayStride;
		} varPropValues;
		GLenum variableProps[] {
			GL_NAME_LENGTH,
			GL_OFFSET,
			GL_ARRAY_SIZE,
			GL_ARRAY_STRIDE,
			GL_TOP_LEVEL_ARRAY_STRIDE
		};

		for (int bufVarIndex = 0; bufVarIndex <  propValues.bufferVariableCount; bufVarIndex++) {
			glGetProgramResourceiv(programHandle, GL_BUFFER_VARIABLE, bufferVars[bufVarIndex], sizeof(variableProps) / sizeof(GLenum), variableProps, sizeof(variableProps) / sizeof(GLenum), nullptr, (int*) &varPropValues);

			char varNameBuf[varPropValues.nameLength];
			
			glGetProgramResourceName(programHandle, GL_BUFFER_VARIABLE, bufferVars[bufVarIndex], varPropValues.nameLength, nullptr, varNameBuf);

			bool isLastVariable = bufVarIndex == propValues.bufferVariableCount - 1;

//			spdlog::info(" {}{}┬─Variable {} : {}", isLast ? " " : "│", isLastVariable ? "└" : "├", bufVarIndex, std::string(varNameBuf));
//			spdlog::info(" {}{}├──Offset: {}", isLast ? " " : "│", isLastVariable ? " " : "│", varPropValues.offset);
//			spdlog::info(" {}{}├──Array size: {}", isLast ? " " : "│", isLastVariable ? " " : "│", varPropValues.arraySize);
//			spdlog::info(" {}{}├──Array stride: {}", isLast ? " " : "│", isLastVariable ? " " : "│", varPropValues.arrayStride);
//			spdlog::info(" {}{}└──Top level array stride: {}", isLast ? " " : "│", isLastVariable ? " " : "│", varPropValues.topLevelArrayStride);
		}

		this->storageBuffers.push_back({ std::string(nameBuf), propValues.binding, 0 });
	}
}

// void UniformSpec::CreateFrom(GLuint programHandle) {
// 		struct {
// 			int nameLength;
// 			int offset;
// 			int arraySize;
// 			int arrayStride;
// 			int topLevelArrayStride;
// 		} varPropValues;
// 		GLenum variableProps[] {
// 			GL_NAME_LENGTH,
// 			GL_OFFSET,
// 			GL_ARRAY_SIZE,
// 			GL_ARRAY_STRIDE,
// 			GL_TOP_LEVEL_ARRAY_STRIDE
// 		};

// 		for (int varIndex = 0; varIndex < propValues.bufferVariableCount; varIndex++) {
// 			glGetProgramResourceiv(programHandle, GL_BUFFER_VARIABLE, bufferVars[varIndex], 5, variableProps, 5, nullptr, (int*) &varPropValues);

// 			char varNameBuf[varPropValues.nameLength];
			
// 			glGetProgramResourceName(programHandle, GL_BUFFER_VARIABLE, bufferVars[varIndex], varPropValues.nameLength, nullptr, varNameBuf);

// 			spdlog::info(" variable {}: name {}, index {}, offset {}, arraySize {}, arrayStride {}", 
// 				varIndex, std::string(varNameBuf), bufferVars[varIndex], varPropValues.offset, varPropValues.arraySize, varPropValues.arraySize == 0 ? varPropValues.arrayStride : varPropValues.topLevelArrayStride
// 			);
// 		}
// 	}
// }

UniformSpec::UniformSpec() { }

UniformSpec::UniformSpec(const ShaderProgram* program) {
	GLuint handle = program->GetHandle();

	CreateFrom(handle);
}

UniformSpec::UniformSpec(const ComputeShaderProgram* program) {
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