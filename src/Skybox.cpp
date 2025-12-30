#include <Skybox.h>

#include <Resources.h>

Mesh* Skybox::skyMesh = nullptr;
Skybox* Skybox::currentSkybox = nullptr;

Skybox::Skybox(Material* skyMaterial):
skyMaterial(skyMaterial) {
	if (!skyMesh) {
		skyMesh = Resources::Get<Mesh>("./res/models/sky.obj");
	}

	SetAsCurrentSkybox();
}

Material* Skybox::GetSKyMaterial() {
	return this->skyMaterial;
}
Mesh* Skybox::GetSkyMesh() {
	return skyMesh;
}

Skybox* Skybox::GetCurrentSkybox() {
	return currentSkybox;
}

void Skybox::SetAsCurrentSkybox() {
	currentSkybox = this;
}
