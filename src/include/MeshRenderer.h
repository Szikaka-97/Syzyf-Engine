#pragma once

#include <vector>

#include <GameObject.h>
#include <Debug.h>

class Mesh;
class Material;

enum MaskEffectBits : uint8_t {
    None = 0,
    XRay = 1,
    Outline = 1 << 1,
    Jfa = 1 << 2, 
};

class MeshRenderer : public GameObject, public ImGuiDrawable {
public:
    uint8_t maskFlags = MaskEffectBits::None;
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

	int GetMaterialCount() const;

	Material* GetMaterial(int materialIndex = 0);

	void SetMaterial(Material* newMaterial, int materialIndex = 0);

	void Render() const;

	virtual void DrawImGui();
};
