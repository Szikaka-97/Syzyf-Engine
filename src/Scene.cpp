#include <Scene.h>

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <spdlog/spdlog.h>
#include <GameObject.h>

SceneNode::SceneNode(Scene* scene) :
scene(scene),
transform(),
children(),
parent(nullptr) {
	this->transform.parent = this;
}

SceneNode::~SceneNode() {
	spdlog::info("Deleting node");

	int objectsCount = this->objects.size();
	GameObject* objectsCopy[objectsCount];

	std::copy(this->objects.begin(), this->objects.end(), objectsCopy);

	for (int i = 0; i < objectsCount; i++) {
		delete objectsCopy[i];
	}

	this->SetParent(nullptr);

	int childrenCount = this->children.size();
	SceneNode* childrenCopy[childrenCount];

	std::copy(this->children.begin(), this->children.end(), childrenCopy);

	for (int i = 0; i < childrenCount; i++) {
		delete childrenCopy[i];
	}

	this->scene->DeleteNodeInternal(this);
}

void SceneNode::RecalculateTransform() {
	if (this->transform.LocalTransform().IsDirty()) {
		if (this->parent) {
			this->transform.GlobalTransform() = this->parent->GlobalTransform().Value() * this->transform.LocalTransform().Value();
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

SceneTransform& SceneNode::GetTransform() {
	if (this->transform.IsDirty()) {
		RecalculateTransform();
	}

	return this->transform;
}

SceneTransform::TransformAccess& SceneNode::LocalTransform() {
	return this->GetTransform().LocalTransform();
}

SceneTransform::TransformAccess& SceneNode::GlobalTransform() {
	return this->GetTransform().GlobalTransform();
}

Scene* SceneNode::GetScene() {
	return this->scene;
}

const std::vector<SceneNode*> SceneNode::GetChildren() {
	return this->children;
}

SceneNode* SceneNode::GetParent() {
	return this->parent;
}

void SceneNode::SetParent(SceneNode* newParent) {
	if (this->parent) {
		auto posInParentChildren = std::find(this->parent->children.begin(), this->parent->children.end(), this);
		if (posInParentChildren != this->parent->children.end()) {
			this->parent->children.erase(posInParentChildren);
		}
	}

	this->parent = newParent;

	if (this->parent) {
		this->parent->children.push_back(this);
	}
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

void SceneNode::DeleteObject(GameObject* obj) {
	this->objects.erase(std::find(this->objects.begin(), this->objects.end(), obj));

	this->scene->DeleteObjectInternal(obj);
}

Scene::Scene() :
updateable(),
renderable(),
root(new SceneNode(this)),
graphics(new SceneGraphics()) { }

void Scene::MessageReceiver::Message() {
	(*this->objPtr.*this->methodPtr)();
}

void Scene::DeleteObjectInternal(GameObject* obj) {
	this->updateable.remove_if( [obj](const MessageReceiver& msgRcvr) {
		return msgRcvr.objPtr == obj;
	} );

	this->renderable.remove_if( [obj](const MessageReceiver& msgRcvr) {
		return msgRcvr.objPtr == obj;
	} );
}

void Scene::DeleteNodeInternal(SceneNode* node) {
	if (node == this->root) {
		this->root = new SceneNode(this);
	}
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

SceneGraphics* Scene::GetGraphics() {
	return this->graphics;
}

void Scene::DeleteObject(GameObject* obj) {
	delete obj;
}

void Scene::DeleteNode(SceneNode* node) {
	delete node;
}

void Scene::Update() {
	for (auto& msgObject : this->updateable) {
		msgObject.Message();
	}
}

void Scene::Render() {
	for (auto& msgObject : this->renderable) {
		msgObject.Message();
	}

	this->graphics->Render();
}