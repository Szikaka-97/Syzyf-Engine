#include <GameObject.h>

#include <Scene.h>

Transform& GameObject::GetTransform() const {
	return this->node->GetTransform();
}

Transform::TransformAccess& GameObject::GlobalTransform() const {
	return this->node->GetTransform().GlobalTransform();
}
Transform::TransformAccess& GameObject::LocalTransform() const {
	return this->node->GetTransform().LocalTransform();
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

Transform::TransformAccess& GameObject::GlobalTransform() {
	return this->node->GetTransform().GlobalTransform();
}
Transform::TransformAccess& GameObject::LocalTransform() {
	return this->node->GetTransform().LocalTransform();
}

SceneNode* GameObject::GetNode() {
	return this->node;
}

Scene* GameObject::GetScene() {
	return this->node->GetScene();
}
