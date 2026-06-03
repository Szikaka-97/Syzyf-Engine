#include <Skybox.h>

#include <Resources.h>
#include <Graphics.h>

Mesh* Skybox::skyMesh = nullptr;
Skybox* Skybox::currentSkybox = nullptr;

Skybox::Skybox():
skyMaterial(nullptr) {
	if (!skyMesh) {
		skyMesh = GetScene()->Resources()->Get<Mesh>("./res/models/sky.obj");
	}
}

Skybox::Skybox(Material* skyMaterial):
skyMaterial(skyMaterial) {
	if (!skyMesh) {
		skyMesh = GetScene()->Resources()->Get<Mesh>("./res/models/sky.obj");
	}

	SetAsCurrentSkybox();
}

void Skybox::OnEnable() {
	if (GetScene()->GetGraphics()->GetActiveSkybox() == nullptr) {
		GetScene()->GetGraphics()->SetActiveSkybox(this);
	}
}
void Skybox::OnDisable() {
	if (GetScene()->GetGraphics()->GetActiveSkybox() == this) {
		GetScene()->GetGraphics()->SetActiveSkybox(nullptr);
	}
}

Material* Skybox::GetSkyMaterial() {
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
