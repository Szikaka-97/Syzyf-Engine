#include "Shader.h"
#include "Texture.h"
#include "UniformSpec.h"
#include "imgui.h"
#include <MeshRenderer.h>

#include <glad/glad.h>
#include <Scene.h>
#include <Graphics.h>
#include <glm/gtc/matrix_access.hpp>

Texture* textureClipboard = nullptr;

MeshRenderer::MeshRenderer():
mesh(),
materials(0) { }

MeshRenderer::MeshRenderer(Mesh* mesh, Material* material):
materials() {
	SetMesh(mesh);
	SetMaterial(material);
}

MeshRenderer::MeshRenderer(Mesh* mesh, const std::vector<Material*>& materials):
materials(materials) {
	SetMesh(mesh);
}

Mesh* MeshRenderer::GetMesh() {
	return this->mesh;
}

void MeshRenderer::SetMesh(Mesh* newMesh) {
	this->mesh = newMesh;
	
	if (this->mesh == nullptr) {
		return;
	}

	std::vector<Material*> newMaterials{newMesh->GetMaterialsCount()};
	int materialsToCopy = std::min(newMesh->GetMaterialsCount(), (unsigned int) this->materials.size());
	for (int i = 0; i < materialsToCopy; i++) {
		newMaterials[i] = this->materials[i];
	}

	this->materials = newMaterials;
}

Material* MeshRenderer::GetMaterial(int materialIndex) {
	if (materialIndex < 0 || this->mesh->GetMaterialsCount() <= materialIndex) {
		return nullptr;
	}

	return this->materials[materialIndex];
}

void MeshRenderer::SetMaterial(Material* newMaterial, int materialIndex) {
	if (materialIndex < 0 || this->mesh->GetMaterialsCount() <= materialIndex) {
		return;
	}

	this->materials[materialIndex] = newMaterial;
}

void MeshRenderer::Render() const {
	this->GetScene()->GetGraphics()->DrawMesh(const_cast<MeshRenderer*>(this));
}

void MeshRenderer::DrawImGui() {
	if (ImGui::TreeNode(std::format("SubMesh count: {}", this->mesh->GetSubMeshCount()).c_str())) {
		ImGui::TreePop();
	}
	if (ImGui::TreeNode(std::format("Material count: {}", this->materials.size()).c_str())) {
		for (int i = 0; i < this->materials.size(); i++) {
			Material* mat = this->materials[i];

			ImGui::PushID(i);

			if (ImGui::TreeNode(std::format("{}", i).c_str())) {
				if (ImGui::TreeNode(std::format("Shader").c_str())) {
					const ShaderProgram* shader = mat->GetShader();

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
					for (int j = 0; j < mat->GetUniforms()->VariableCount(); j++) {
						auto& uniform = mat->GetUniforms()->VariableAt(j);

						if (uniform.name.starts_with("Builtin")) {
							continue;
						}

						ImGui::PushID(j);

						ImGui::Text("%i: %s", uniform.binding, uniform.name.c_str());

						switch (uniform.type) {
						case UniformSpec::UniformType::Float1: {
							float val = mat->GetValue<float>(j);
							float origVal = val;
							
							ImGui::InputFloat(uniform.name.c_str(), &val);

							if (val != origVal) {
								mat->SetValue(j, val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Float2: {
							glm::vec2 val = mat->GetValue<glm::vec2>(j);
							glm::vec2 origVal = val;
							
							ImGui::InputFloat2(uniform.name.c_str(), &val[0]);

							if (val != origVal) {
								mat->SetValue(j, val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Float3: {
							glm::vec3 val = mat->GetValue<glm::vec3>(j);
							glm::vec3 origVal = val;
							
							ImGui::InputFloat3(uniform.name.c_str(), &val[0]);

							if (val != origVal) {
								mat->SetValue(j, val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Float4: {
							glm::vec4 val = mat->GetValue<glm::vec4>(j);
							glm::vec4 origVal = val;
							
							ImGui::InputFloat4(uniform.name.c_str(), &val[0]);

							if (val != origVal) {
								mat->SetValue(j, val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Uint1: {
							int val = mat->GetValue<unsigned int>(j);
							int origVal = val;
							
							ImGui::InputInt(uniform.name.c_str(), &val);

							if (val != origVal) {
								mat->SetValue(j, (unsigned int) val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Uint2: {
							glm::ivec2 val = mat->GetValue<glm::uvec2>(j);
							glm::ivec2 origVal = val;
							
							ImGui::InputInt2(uniform.name.c_str(), &val[0]);

							if (val != origVal) {
								mat->SetValue(j, (glm::uvec2) val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Uint3: {
							glm::ivec3 val = mat->GetValue<glm::uvec3>(j);
							glm::ivec3 origVal = val;
							
							ImGui::InputInt3(uniform.name.c_str(), &val[0]);

							if (val != origVal) {
								mat->SetValue(j, (glm::uvec3) val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Uint4: {
							glm::ivec4 val = mat->GetValue<glm::uvec4>(j);
							glm::ivec4 origVal = val;
							
							ImGui::InputInt4(uniform.name.c_str(), &val[0]);

							if (val != origVal) {
								mat->SetValue(j, (glm::uvec4) val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Matrix3x3: {
							glm::mat3 val = mat->GetValue<glm::mat3>(j);
							glm::mat3 origVal = val;
							
							glm::vec3 row0 = glm::row(val, 0);
							glm::vec3 row1 = glm::row(val, 1);
							glm::vec3 row2 = glm::row(val, 2);

							ImGui::InputFloat3(uniform.name.c_str(), &row0[0]);
							ImGui::InputFloat3(" ", &row1[0]);
							ImGui::InputFloat3("", &row2[0]);

							val[0][0] = row0[0];
							val[0][1] = row1[0];
							val[0][2] = row2[0];
							val[1][0] = row0[1];
							val[1][1] = row1[1];
							val[1][2] = row2[1];
							val[2][0] = row0[2];
							val[2][1] = row1[2];
							val[2][2] = row2[2];

							if (val != origVal) {
								mat->SetValue(j, val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Matrix4x4: {
							glm::mat4 val = mat->GetValue<glm::mat4>(j);
							glm::mat4 origVal = val;
							
							glm::vec4 row0 = glm::row(val, 0);
							glm::vec4 row1 = glm::row(val, 1);
							glm::vec4 row2 = glm::row(val, 2);
							glm::vec4 row3 = glm::row(val, 3);

							ImGui::InputFloat4(uniform.name.c_str(), &row0[0]);
							ImGui::InputFloat4(" ", &row1[0]);
							ImGui::InputFloat4("", &row2[0]);
							ImGui::InputFloat4("  ", &row3[0]);

							val[0][0] = row0[0];
							val[0][1] = row1[0];
							val[0][2] = row2[0];
							val[0][3] = row3[0];
							val[1][0] = row0[1];
							val[1][1] = row1[1];
							val[1][2] = row2[1];
							val[1][3] = row3[1];
							val[2][0] = row0[2];
							val[2][1] = row1[2];
							val[2][2] = row2[2];
							val[2][3] = row3[2];
							val[3][0] = row0[3];
							val[3][1] = row1[3];
							val[3][2] = row2[3];
							val[3][3] = row3[3];

							if (val != origVal) {
								mat->SetValue(j, val);
							}
							
							break;
						}
						case UniformSpec::UniformType::Sampler2D:
						case UniformSpec::UniformType::Cubemap: {
							Texture* tex = nullptr;

							tex = mat->GetValue<Texture2D>(j);
							if (tex == nullptr) {
								tex = mat->GetValue<Cubemap>(j);
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
										mat->SetValue<Texture2D>(j, (Texture2D*) textureClipboard);
									}
									else {
										mat->SetValue<Cubemap>(j, (Cubemap*) textureClipboard);
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

				ImGui::TreePop();
			}


			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}