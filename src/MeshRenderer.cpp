#include <MeshRenderer.h>

#include <glad/glad.h>
#include <Scene.h>

MeshRenderer::MeshRenderer():
mesh(),
materials(0),
uniformBufferHandle(0),
dirty(false) {
	ResetUniformBuffer();
}

MeshRenderer::MeshRenderer(Mesh* mesh, Material* material):
materials(),
uniformBufferHandle(0),
dirty(true) {
	SetMesh(mesh);
	SetMaterial(material);

	ResetUniformBuffer();
}

void MeshRenderer::ResetUniformBuffer() {
	if (this->uniformBufferHandle) {
		glDeleteBuffers(1, &this->uniformBufferHandle);
	}

	glGenBuffers(1, &this->uniformBufferHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, this->uniformBufferHandle);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

GLuint MeshRenderer::GetUniformBufferHandle() {
	return this->uniformBufferHandle;
}

Mesh* MeshRenderer::GetMesh() {
	return this->mesh;
}

void MeshRenderer::SetMesh(Mesh* newMesh) {
	this->mesh = newMesh;
	
	this->dirty = true;

	if (this->mesh == nullptr) {
		return;
	}

	std::vector<Material*> newMaterials{newMesh->GetMaterialsCount()};
	int materialsToCopy = std::min(newMesh->GetMaterialsCount(), (int) this->materials.size());
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

	this->dirty = true;
}

void MeshRenderer::Render() const {
	this->GetScene()->GetGraphics()->DrawMesh(const_cast<MeshRenderer*>(this));
}