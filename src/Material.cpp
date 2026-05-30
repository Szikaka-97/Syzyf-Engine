#include "Debug.h"
#include "UniformSpec.h"
#include <Material.h>

#include <imgui.h>
#include <malloc.h>

std::vector<Material*> Material::allMaterials;
Texture* textureClipboard = nullptr;

void ShaderVariableStorage::Bind() const {
	int samplerIndex = 0;

	for (unsigned int i = 0; i < this->uniformSpec->VariableCount(); i++) {
		int offset = this->uniformSpec->VariableAt(i).offset;

		switch (this->uniformSpec->VariableAt(i).type) {
    case UniformSpec::UniformType::Bool:
      glUniform1i(this->uniformSpec->VariableAt(i).binding, GetValue<bool>(i));
      break;
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
		case UniformSpec::UniformType::Uint4:
			glUniform4uiv(this->uniformSpec->VariableAt(i).binding, 1, &GetValue<glm::uvec4>(i)[0]);
			break;
		case UniformSpec::UniformType::Matrix3x3:
			glUniformMatrix3fv(this->uniformSpec->VariableAt(i).binding, 1, false, &GetValue<glm::mat3>(i)[0][0]);
			break;
		case UniformSpec::UniformType::Matrix4x4:
			glUniformMatrix4fv(this->uniformSpec->VariableAt(i).binding, 1, false, &GetValue<glm::mat4>(i)[0][0]);
			break;
		case UniformSpec::UniformType::Sampler2D:
		{
			UniformSpec::TextureUniform<Texture2D> imageTex = GetValue<Texture2D>(i);

			GLuint imageTexHandle = 0;

			if (imageTex.tex) {
				if (imageTex.tex->IsDirty()) {
					imageTex.tex->Update();
				}
				
				imageTexHandle = imageTex.tex->GetHandle();
			}
			
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			glBindTexture(GL_TEXTURE_2D, imageTexHandle);
			glUniform1i(this->uniformSpec->VariableAt(i).binding, samplerIndex);

			samplerIndex++;

			break;
		}
		case UniformSpec::UniformType::Sampler3D:
		{
			UniformSpec::TextureUniform<Texture3D> imageTex = GetValue<Texture3D>(i);

			GLuint imageTexHandle = 0;

			if (imageTex.tex) {
				if (imageTex.tex->IsDirty()) {
					imageTex.tex->Update();
				}
				
				imageTexHandle = imageTex.tex->GetHandle();
			}
			
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			glBindTexture(GL_TEXTURE_3D, imageTexHandle);
			glUniform1i(this->uniformSpec->VariableAt(i).binding, samplerIndex);

			samplerIndex++;

			break;
		}
		case UniformSpec::UniformType::Cubemap:
		{
			UniformSpec::TextureUniform<Cubemap> cubeTex = GetValue<Cubemap>(i);

			GLuint cubeTexHandle = 0;

			if (cubeTex.tex) {
				if (cubeTex.tex->IsDirty()) {
					cubeTex.tex->Update();
				}
				
				cubeTexHandle = cubeTex.tex->GetHandle();
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
			UniformSpec::TextureUniform<Texture2D> imageTex = GetValue<Texture2D>(i);

			GLuint imageTexHandle = 0;
			GLenum imageFormat = GL_RGBA16F;

			if (imageTex.tex) {
				if (imageTex.tex->IsDirty()) {
					imageTex.tex->Update();
				}
				
				imageTexHandle = imageTex.tex->GetHandle();
				imageFormat = Texture::CalcInternalFormat({
					.channels = imageTex.tex->GetChannels(),
					.colorSpace = imageTex.tex->GetColorSpace(),
					.format = imageTex.tex->GetFormat(),
				});

				glUniform1i(this->uniformSpec->VariableAt(i).binding, samplerIndex);
			}

			glBindImageTexture(samplerIndex, imageTexHandle, imageTex.level, false, 0, GL_READ_WRITE, imageFormat);

			samplerIndex++;

			break;
		}
		case UniformSpec::UniformType::ImageCube:
		{
			UniformSpec::TextureUniform<Cubemap> cubeTex = GetValue<Cubemap>(i);

			GLuint imageTexHandle = 0;
			GLenum imageFormat = GL_RGBA16F;

			if (cubeTex.tex) {
				if (cubeTex.tex->IsDirty()) {
					cubeTex.tex->Update();
				}
				
				imageTexHandle = cubeTex.tex->GetHandle();
				imageFormat = Texture::CalcInternalFormat({
					.channels = cubeTex.tex->GetChannels(),
					.colorSpace = cubeTex.tex->GetColorSpace(),
					.format = cubeTex.tex->GetFormat(),
				});

				glUniform1i(this->uniformSpec->VariableAt(i).binding, samplerIndex);
			}

			glBindImageTexture(samplerIndex, imageTexHandle, cubeTex.level, true, 0, GL_READ_WRITE, imageFormat);

			samplerIndex++;

			break;
		}
		case UniformSpec::UniformType::Unsupported:
		default:
			break;
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

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, this->uniformSpec->StorageBufferAt(i).binding, storageBufferData.bufferHandle);
	}
}

ShaderVariableStorage::ShaderVariableStorage(const UniformSpec& uniformSpec):
uniformSpec(&uniformSpec) {
	unsigned int variableBufferSize = uniformSpec.GetBufferSize();
	
	this->dataBuffer.resize(variableBufferSize);
	memset(this->dataBuffer.data(), 0, variableBufferSize);

	int uniformBuffersCount = uniformSpec.UniformBuffersCount();

	this->uniformBuffers = new BufferPair[uniformBuffersCount];
	GLuint* uniformBufferHandles = (GLuint*) alloca(sizeof(GLuint) * uniformBuffersCount);

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

void ShaderVariableStorage::RefreshVariables() {
	std::vector<unsigned char> newBuffer;
	newBuffer.resize(this->uniformSpec->GetBufferSize());

	memcpy(newBuffer.data(), this->dataBuffer.data(), std::min(newBuffer.size(), this->dataBuffer.size()));

	this->dataBuffer = newBuffer;
}

Material::Material(const ShaderProgram* shader):
shader(shader),
shaderVariables(shader->GetUniforms()) {
	allMaterials.push_back(this);
}

void Material::OnReloadShader(ShaderProgram* shader) {
	for (auto mat : allMaterials) {
		if (mat->shader == shader) {
			mat->shaderVariables.RefreshVariables();
		}
	}
}

void Material::Bind(const ShaderProgram* targetProgram) const {
	if (targetProgram != nullptr) {
        glUseProgram(targetProgram->GetHandle());
    } else {
        glUseProgram(this->shader->GetHandle());
    }

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

template<> bool Debug::Property<Material>(Material& mat, const std::string &name) {
	if (ImGui::TreeNode(std::format("Shader").c_str())) {
		const ShaderProgram* shader = mat.GetShader();

		ImGui::Text("Vertex shader: %s", shader->GetVertexShader().GetName().c_str());
		ImGui::Text("Geometry shader: %s", shader->GetGeometryShader().GetName().c_str());
		ImGui::Text("Tess control shader: %s", shader->GetTessCtrlShader().GetName().c_str());
		ImGui::Text("Tess evaluation shader: %s", shader->GetTessEvalShader().GetName().c_str());
		ImGui::Text("Fragment shader: %s", shader->GetPixelShader().GetName().c_str());

		if (ImGui::Button("Reload")) {
			const_cast<ShaderProgram*>(shader)->Reload();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode(std::format("Properties").c_str())) {
		for (int j = 0; j < mat.GetUniforms()->VariableCount(); j++) {
			auto& uniform = mat.GetUniforms()->VariableAt(j);

			if (uniform.name.starts_with("Builtin")) {
				continue;
			}

			ImGui::PushID(j);

			ImGui::Text("%i: %s", uniform.binding, uniform.name.c_str());

			switch (uniform.type) {
			case UniformSpec::UniformType::Float1: {
				float val = mat.GetValue<float>(j);

				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Float2: {
				glm::vec2 val = mat.GetValue<glm::vec2>(j);
				
				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Float3: {
				glm::vec3 val = mat.GetValue<glm::vec3>(j);
				
				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Float4: {
				glm::vec4 val = mat.GetValue<glm::vec4>(j);
				
				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Uint1: {
				unsigned int val = mat.GetValue<unsigned int>(j);
				
				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Uint2: {
				glm::uvec2 val = mat.GetValue<glm::uvec2>(j);

				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Uint3: {
				glm::uvec3 val = mat.GetValue<glm::uvec3>(j);
				
				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Uint4: {
				glm::uvec4 val = mat.GetValue<glm::uvec4>(j);
				
				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Matrix3x3: {
				glm::mat3 val = mat.GetValue<glm::mat3>(j);

				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Matrix4x4: {
				glm::mat4 val = mat.GetValue<glm::mat4>(j);

				if (Debug::Property(val, uniform.name)) {
					mat.SetValue(j, val);
				}
				
				break;
			}
			case UniformSpec::UniformType::Sampler2D:
			case UniformSpec::UniformType::Cubemap: {
				Texture* tex = nullptr;

				tex = mat.GetValue<Texture2D>(j);
				if (tex == nullptr) {
					tex = mat.GetValue<Cubemap>(j);
				}

				ImGui::LabelText(uniform.name.c_str(), "%i", tex->GetHandle());

				if (tex) {
					if (ImGui::Button("Copy")) {
						textureClipboard = tex;
					}
				}
				else {
					ImGui::Spacing();
				}
				
				ImGui::SameLine();
				
				if (textureClipboard) {
					if (ImGui::Button(std::format("Paste {}", textureClipboard->GetHandle()).c_str())) {
						if (textureClipboard->GetType() == TextureType::Texture2D) {
							mat.SetValue<Texture2D>(j, (Texture2D*) textureClipboard);
						}
						else {
							mat.SetValue<Cubemap>(j, (Cubemap*) textureClipboard);
						}
					}
				}
				else {
					ImGui::Spacing();
				}

				break;
			}
			case UniformSpec::UniformType::Image2D:
			case UniformSpec::UniformType::ImageCube:
			case UniformSpec::UniformType::UImage2D:
			case UniformSpec::UniformType::Unsupported:
				break;
			}

			ImGui::PopID();
		}

		ImGui::TreePop();
	}

	return false;
}
