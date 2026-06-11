#pragma once

#include <vector>

#include <GameObject.h>
#include <Debug.h>

class Mesh;
class Material;
class SkeletonComponent;

enum MaskEffectBits : uint8_t {
	None = 0,
	XRay = 1,
	Outline = 2,
	Jfa = 4, 
	Effect4 = 8, 
	Effect5 = 16, 
	Effect6 = 32, 
	Effect7 = 64, 
	Effect8 = 128, 
};

class MeshRenderer : public GameObject, public ImGuiDrawable {
public:
	uint8_t maskFlags = MaskEffectBits::None;
private:
	serialized Mesh* mesh;
	serialized std::vector<Material*> materials;
	SkeletonComponent* skeleton;

	void ResetUniformBuffer();
	void DrawMaskEffectCheckbox(MaskEffectBits maskEffect);
public:
	MeshRenderer();
	MeshRenderer(Mesh* mesh, Material* material);
	MeshRenderer(Mesh* mesh, const std::vector<Material*>& materials);

	Mesh* GetMesh();
	void SetMesh(Mesh* newMesh);

	int GetMaterialCount() const;

	Material* GetMaterial(int materialIndex = 0);

	void SetMaterial(Material* newMaterial, int materialIndex = 0);

	SkeletonComponent* GetSkeleton();
	void SetSkeleton(SkeletonComponent* skeleton);

	void Render() const;

	virtual void DrawImGui();
};
