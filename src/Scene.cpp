#include <Scene.h>

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include <spdlog/spdlog.h>
#include <GameObject.h>

SceneNode::SceneNode(Scene* scene) :
scene(scene),
transform(),
children(),
parent(nullptr) {
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

Scene::Scene() :
updateable(),
renderable(),
root(new SceneNode(this)),
graphics(new SceneGraphics()) { }

void Scene::MessageReceiver::Message() {
	(*this->objPtr.*methodPtr)();
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