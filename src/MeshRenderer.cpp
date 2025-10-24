#include <MeshRenderer.h>

#include <glad/glad.h>
#include <Scene.h>

MeshRenderer::MeshRenderer():
mesh(),
material(nullptr),
uniformBufferHandle(0),
dirty(false) {
	ResetUniformBuffer();
}

MeshRenderer::MeshRenderer(Mesh mesh, Material* material):
mesh(mesh),
material(material),
uniformBufferHandle(0),
dirty(true) {
	// if (this->material->GetShader()->GetVertexSpec() != this->mesh->GetMeshSpec()) {
	// 	this->mesh = this->mesh->GetVariant(this->material->GetShader()->GetVertexSpec());
	// }

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

Mesh MeshRenderer::GetMesh() {
	return this->mesh;
}

void MeshRenderer::SetMesh(Mesh newMesh) {
	// if (this->material->GetShader()->GetVertexSpec() != newMesh->GetMeshSpec()) {
	// 	newMesh = newMesh->GetVariant(this->material->GetShader()->GetVertexSpec());
	// }

	this->mesh = newMesh;

	this->dirty = true;
}

Material* MeshRenderer::GetMaterial() {
	return this->material;
}

void MeshRenderer::SetMaterial(Material* newMaterial) {
	this->material = newMaterial;

	// if (this->material->GetShader()->GetVertexSpec() != this->mesh->GetMeshSpec()) {
	// 	this->mesh = this->mesh->GetVariant(this->material->GetShader()->GetVertexSpec());
	// }

	this->dirty = true;
}

void MeshRenderer::Render() const {
	this->GetScene()->GetGraphics()->DrawMesh(const_cast<MeshRenderer*>(this));
}