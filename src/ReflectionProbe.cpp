#include <ReflectionProbe.h>

#include <Texture.h>
#include <Resources.h>
#include <Mesh.h>
#include <Material.h>
#include <Graphics.h>

#include <imgui.h>

ReflectionProbe::ReflectionProbe():
dirty(true),
irradianceMap(nullptr),
prefilterMap(nullptr) {
	ShaderProgram* cubemapGizmoShader = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/lit.vert")
	.WithPixelShader("./res/shaders/cubemap.frag")
	.Link();
	
	this->gizmoMaterial = new Material(cubemapGizmoShader);
}

void ReflectionProbe::Regenerate() {
	this->dirty = true;
}

Mesh* cubemapGizmoMesh = nullptr;

Cubemap* ReflectionProbe::GetIrradianceMap() {
	return this->irradianceMap;
}
Cubemap* ReflectionProbe::GetPrefilterMap() {
	return this->prefilterMap;
}

void ReflectionProbe::DrawGizmos() {
	if (cubemapGizmoMesh == nullptr) {
		cubemapGizmoMesh = GetScene()->Resources()->Get<Mesh>("./res/models/sphere.obj");
	}

	if (this->dirty) {
		return;
	}

	this->gizmoMaterial->SetValue("cubemap", this->prefilterMap);

	// GetScene()->GetGraphics()->DrawGizmoMesh(cubemapGizmoMesh, 0, this->gizmoMaterial, GlobalTransform().Value());
}

void ReflectionProbe::DrawImGui() {
	ImGui::Text("Status: %s", this->dirty ? "Dirty" : "Clean");

	if (ImGui::Button("Recalculate")) {
		this->Regenerate();
	}
}