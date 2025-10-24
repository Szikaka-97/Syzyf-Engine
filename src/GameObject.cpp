#include <GameObject.h>

#include <Scene.h>

Transform& GameObject::GetTransform() const {
	return this->node->GetTransform();
}

SceneNode* GameObject::GetNode() const {
	return this->node;
}

Scene* GameObject::GetScene() const {
	return this->node->GetScene();
}

Transform& GameObject::GetTransform() {
	return this->node->GetTransform();
}

SceneNode* GameObject::GetNode() {
	return this->node;
}

Scene* GameObject::GetScene() {
	return this->node->GetScene();
}
