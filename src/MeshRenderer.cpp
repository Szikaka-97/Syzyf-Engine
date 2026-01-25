#include <MeshRenderer.h>

#include <glad/glad.h>
#include <Scene.h>
#include <Graphics.h>

MeshRenderer::MeshRenderer():
mesh(),
materials(0) { }

MeshRenderer::MeshRenderer(Mesh* mesh, Material* material):
materials() {
	SetMesh(mesh);
	SetMaterial(material);
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