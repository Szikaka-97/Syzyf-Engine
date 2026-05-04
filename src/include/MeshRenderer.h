#pragma once

#include <vector>

#include <GameObject.h>
#include <Debug.h>

class Mesh;
class Material;

class MeshRenderer : public GameObject, public ImGuiDrawable {
public:
    bool hasOutline = false;
    bool hasXray = false;
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

	int GetMaterialCount() const;

	Material* GetMaterial(int materialIndex = 0);

	void SetMaterial(Material* newMaterial, int materialIndex = 0);

	void Render() const;

	virtual void DrawImGui();
};
