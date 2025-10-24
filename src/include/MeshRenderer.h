#pragma once

#include <GameObject.h>
#include <Mesh.h>
#include <Material.h>

class MeshRenderer : public GameObject {
private:
	Mesh mesh;
	Material* material;

	GLuint uniformBufferHandle;
	bool dirty;

	void ResetUniformBuffer();
public:
	MeshRenderer();
	MeshRenderer(Mesh mesh, Material* material);

	Mesh GetMesh();
	void SetMesh(Mesh newMesh);

	GLuint GetUniformBufferHandle();

	Material* GetMaterial();
	void SetMaterial(Material* newMaterial);

	void Render() const;
};