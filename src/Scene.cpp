#include <Scene.h>

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include <GameObject.h>

SceneNode::SceneNode(Scene* scene) :
scene(scene),
transform(),
children() {
	this->transform.parent = this;
}

void SceneNode::RecalculateTransform() {
	if (this->transform.LocalTransform().IsDirty()) {
		if (this->parent) {
			this->transform.GlobalTransform() = this->transform.LocalTransform().Value() * this->parent->GlobalTransform().Value();
		}
		else {
			this->transform.GlobalTransform() = this->transform.LocalTransform().Value();
		}
	}
	else if (this->transform.GlobalTransform().IsDirty()) {
		if (this->parent) {
			this->transform.LocalTransform() = this->transform.GlobalTransform().Value() * glm::inverse(this->parent->GlobalTransform().Value());
		}
		else {
			this->transform.LocalTransform() = this->transform.GlobalTransform().Value();
		}
	}

	this->transform.ClearDirty();
}

Transform& SceneNode::GetTransform() {
	if (this->transform.IsDirty()) {
		RecalculateTransform();
	}

	return this->transform;
}

Transform::TransformAccess& SceneNode::LocalTransform() {
	return this->GetTransform().LocalTransform();
}

Transform::TransformAccess& SceneNode::GlobalTransform() {
	return this->GetTransform().GlobalTransform();
}

const std::vector<SceneNode*> SceneNode::GetChildren() {
	return this->children;
}

SceneNode* SceneNode::GetParent() {
	return this->parent;
}

void SceneNode::SetParent(SceneNode* newParent) {
	if (this->parent) {
		this->parent->children.erase(std::find(this->parent->children.begin(), this->parent->children.end(), this));
	}

	this->parent = newParent;
	this->parent->children.push_back(this);
}

bool SceneNode::IsChildOf(const SceneNode* node) {
	if (this->parent == node) {
		return true;
	}
	return this->parent->IsChildOf(node);
}

void SceneNode::MarkDirty() {
	this->transform.LocalTransform().MarkDirty();

	MarkChildrenDirty();
}

void SceneNode::MarkChildrenDirty() {
	for (auto child : this->children) {
		child->MarkDirty();
	}
}

const std::vector<GameObject*> SceneNode::AttachedObjects() {
	return this->objects;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
T_GO* SceneNode::AddObject() {
	return this->scene->CreateObjectOn<T_GO>(this);
}

Scene::Scene() :
root(new SceneNode(this)) { }

void Scene::MessageReceiver::Message() {
	(*this->objPtr.*methodPtr)();
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateAwakeable(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Awakeable<T_GO>
bool Scene::TryCreateAwakeable(T_GO* object) {
#warning TODO
	return true;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateUpdateable(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Updateable<T_GO>
bool Scene::TryCreateUpdateable(T_GO* object) {
	this->updateable.push_back({ object, reinterpret_cast<MessageMethod>(&T_GO::Update) });

	return true;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateEnableable(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Enableable<T_GO>
bool Scene::TryCreateEnableable(T_GO* object) {
#warning TODO
	return true;
}

SceneNode* Scene::CreateNode() {
	return CreateNode(this->root);
}
SceneNode* Scene::CreateNode(SceneNode* parent) {
	SceneNode* result = new SceneNode(this);

	if (parent) {
		result->SetParent(parent);
	}
	else {
		result->SetParent(this->root);
	}

	return result;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
T_GO* Scene::CreateObjectOn(SceneNode* node) {
	GameObject* created = new GameObject();
	created->node = node;

	TryCreateAwakeable(created);
	TryCreateEnableable(created);
	TryCreateUpdateable(created);

	return created;
}

void Scene::Update() {
	for (auto& msgObject : this->updateable) {
		msgObject.Message();
	}
}

void Scene::Render() {
#warning TODO
}