#include <SceneComponent.h>

#include <Scene.h>

SceneComponent::SceneComponent(Scene* scene):
scene(scene) { }

SceneComponent::~SceneComponent() = default;

Scene* SceneComponent::GetScene() const {
	return this->scene;
}

void SceneComponent::OnPreUpdate() {}
void SceneComponent::OnPostUpdate() {}

void SceneComponent::OnPreRender() {}
void SceneComponent::OnPostRender() {}

void SceneComponent::DrawImGui() {}

int SceneComponent::Order() {
	return 0;
}
