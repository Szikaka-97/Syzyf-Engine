#include <ReflectionProbe.h>

#include <Texture.h>
#include <Resources.h>
#include <Mesh.h>
#include <Material.h>
#include <Graphics.h>

ReflectionProbe::ReflectionProbe():
dirty(true) {
	this->cubemap = new Cubemap(resolution, resolution, Texture::LDRColorBuffer);
}

void ReflectionProbe::Regenerate() {
	this->dirty = true;
}

Mesh* cubemapGizmoMesh = nullptr;
Material* cubemapGizmoMaterial = nullptr;

void ReflectionProbe::DrawGizmos() {
	if (cubemapGizmoMesh == nullptr) {
		cubemapGizmoMesh = Resources::Get<Mesh>("./res/models/sphere.obj");
	}
	if (cubemapGizmoMaterial == nullptr) {
		ShaderProgram* cubemapGizmoShader = ShaderProgram::Build()
		.WithVertexShader(Resources::Get<VertexShader>("./res/shaders/lit.vert"))
		.WithPixelShader(Resources::Get<PixelShader>("./res/shaders/cubemap.frag"))
		.Link();
		
		cubemapGizmoMaterial = new Material(cubemapGizmoShader);
	}
	
	if (this->dirty) {
		return;
	}

	cubemapGizmoMaterial->SetValue("cubemap", this->cubemap);

	GetScene()->GetGraphics()->DrawGizmoMesh(cubemapGizmoMesh, 0, cubemapGizmoMaterial, GlobalTransform().Value());
}