#include <GameObject.h>

#include <Scene.h>

GameObject::~GameObject() {
	this->node->DeleteObject(this);
}

SceneTransform& GameObject::GetTransform() const {
	return this->node->GetTransform();
}

SceneTransform::TransformAccess& GameObject::GlobalTransform() const {
	return this->node->GetTransform().GlobalTransform();
}
SceneTransform::TransformAccess& GameObject::LocalTransform() const {
	return this->node->GetTransform().LocalTransform();
}

SceneNode* GameObject::GetNode() const {
	return this->node;
}

Scene* GameObject::GetScene() const {
	return this->node->GetScene();
}

SceneTransform& GameObject::GetTransform() {
	return this->node->GetTransform();
}

SceneTransform::TransformAccess& GameObject::GlobalTransform() {
	return this->node->GetTransform().GlobalTransform();
}
SceneTransform::TransformAccess& GameObject::LocalTransform() {
	return this->node->GetTransform().LocalTransform();
}

SceneNode* GameObject::GetNode() {
	return this->node;
}

Scene* GameObject::GetScene() {
	return this->node->GetScene();
}

bool GameObject::IsEnabled() const {
	return this->enabled;
}
void GameObject::SetEnabled(bool enabled) {
	if (enabled == this->enabled) {
		return;
	}

	if (enabled && this->onEnable) {
		(*this.*this->onEnable)();
	}
	else if (!enabled && this->onDisable) {
		(*this.*this->onDisable)();
	}
}
