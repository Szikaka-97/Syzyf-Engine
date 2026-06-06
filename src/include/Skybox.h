#pragma once

#include <glad/glad.h>

#include <GameObject.h>
#include <Mesh.h>
#include <Material.h>

class Skybox : public GameObject {
private:
	static Mesh* skyMesh;
	serialized Material* skyMaterial;

	static Skybox* currentSkybox;
public:
	Skybox();
	Skybox(Material* skyMaterial);

	void OnEnable();
	void OnDisable();

	Material* GetSkyMaterial();
	Mesh* GetSkyMesh();

	static Skybox* GetCurrentSkybox();
	void SetAsCurrentSkybox();
};