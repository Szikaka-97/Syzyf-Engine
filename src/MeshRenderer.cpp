#include "Material.h"
#include "imgui.h"
#include <MeshRenderer.h>

#include <glad/glad.h>
#include <Scene.h>
#include <Graphics.h>
#include <glm/gtc/matrix_access.hpp>
#include <tracy/Tracy.hpp>

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

int MeshRenderer::GetMaterialCount() const {
	return this->mesh->GetMaterialsCount();
}

void MeshRenderer::SetMaterial(Material* newMaterial, int materialIndex) {
	if (materialIndex < 0 || this->mesh->GetMaterialsCount() <= materialIndex) {
		return;
	}

	this->materials[materialIndex] = newMaterial;
}

SkeletonComponent* MeshRenderer::GetSkeleton() {
	return this->skeleton;
}
void MeshRenderer::SetSkeleton(SkeletonComponent* skeleton) {
	this->skeleton = skeleton;
}

void MeshRenderer::Render() const {
	this->GetScene()->GetGraphics()->DrawMesh(const_cast<MeshRenderer*>(this));
}

void MeshRenderer::DrawImGui() {
	if (ImGui::TreeNode(std::format("SubMesh count: {}", this->mesh->GetSubMeshCount()).c_str())) {
		ImGui::TreePop();
	}
   
    if (ImGui::TreeNode("Mask Effects")) {
        for (unsigned int i = 0; i < 8; i++) {
            DrawMaskEffectCheckbox((MaskEffectBits)(MaskEffectBits::XRay << i));
        }
        ImGui::TreePop();
    }

	if (ImGui::TreeNode(std::format("Material count: {}", this->materials.size()).c_str())) {
		for (int i = 0; i < this->materials.size(); i++) {
			Material* mat = this->materials[i];

			ImGui::PushID(i);

			if (ImGui::TreeNode(mat->name.c_str())) {
				Debug::Property(*mat, "");

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}

void MeshRenderer::DrawMaskEffectCheckbox(MaskEffectBits maskEffect) {
    bool hasEffect = (this->maskFlags & maskEffect) != 0;
    if (ImGui::Checkbox(std::format("Effect {}", (unsigned int)maskEffect).c_str(), &hasEffect)) {
        if (hasEffect) this->maskFlags |= maskEffect;
        else this->maskFlags &= ~maskEffect;
    }
}
