#pragma once

#include <vector>

#include <GameObject.h>
#include <Mesh.h>
#include <Material.h>

class MeshRenderer : public GameObject {
private:
	Mesh* mesh;
	std::vector<Material*> materials;

	void ResetUniformBuffer();
public:
	MeshRenderer();
	MeshRenderer(Mesh* mesh, Material* material);
	MeshRenderer(Mesh* mesh, const std::vector<Material*>& materials);

	Mesh* GetMesh();
	void SetMesh(Mesh* newMesh);

	Material* GetMaterial(int materialIndex = 0);

	void SetMaterial(Material* newMaterial, int materialIndex = 0);

	void Render() const;
};