#pragma once

#include <vector>

#include <GameObject.h>
#include <Mesh.h>
#include <Material.h>
#include <Serialized.h>

class MeshRenderer : public GameObject {
private:
	serialized Mesh* mesh;
	serialized std::vector<Material*> materials;

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