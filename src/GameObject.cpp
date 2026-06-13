#include <GameObject.h>

#include <Scene.h>
#include <Light.h>
#include <TypeInfo.h>

GameObject::~GameObject() {
	this->node->DeleteObject(this);
}

int GameObject::GetID() const {
	return this->id;
}

std::string GameObject::GetName() const {
    return TypeInfo::GetTypeInfo(typeid(*this)).name;
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
	return EnabledSelf() && this->node->IsEnabled();
}

bool GameObject::EnabledSelf() const {
	return this->enabled;
}

void GameObject::SetEnabled(bool enabled) {
	if (enabled == this->enabled) {
		return;
	}

	GetScene()->SetGameObjectEnabledInternal(this, enabled);

	this->enabled = enabled;
}

void GameObject::operator delete(GameObject* ptr, std::destroying_delete_t) {
	ptr->GetScene()->QueueDelete(ptr);
}
