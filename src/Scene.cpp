#include <Scene.h>
#include "Messaging.h"
#include "Serialized.h"

#include <malloc.h>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

#include <GameObject.h>
#include <Graphics.h>
#include <InputSystem.h>
#include <Layer.h>
#include <spdlog/spdlog.h>

GameObject* MessagingHelpers_AddObjectToNode(SceneNode* node, const std::string& objectName); // Trust me bro

SceneNode::SceneNode(Scene* scene) :
scene(scene),
transform(),
children(),
parent(nullptr),
disabledState(0),
layer(Layer::Default),
name("") {
	this->transform.parent = this;
}

SceneNode::~SceneNode() {
	int childrenCount = this->children.size();
	SceneNode** childrenCopy = (SceneNode**) alloca(sizeof(SceneNode*) * childrenCount);

	std::copy(this->children.begin(), this->children.end(), childrenCopy);

	for (int i = 0; i < childrenCount; i++) {
		childrenCopy[i]->~SceneNode();
	}

	this->scene->DeleteNodeInternal(this);

	if (this->parent) {
		auto posInParentChildren = std::find(this->parent->children.begin(), this->parent->children.end(), this);

		if (posInParentChildren != this->parent->children.end()) {
			this->parent->children.erase(posInParentChildren);
		}
	}

	std::free(this);
}

void SceneNode::RecalculateTransform() {
	if (this->transform.LocalTransform().IsDirty()) {
		if (this->parent) {
			this->transform.GlobalTransform() =
				this->parent->GlobalTransform().Value() *
				this->transform.LocalTransform().Value();
		} else {
			this->transform.GlobalTransform() =
				this->transform.LocalTransform().Value();
		}
	} else if (this->transform.GlobalTransform().IsDirty()) {
		if (this->parent) {
			this->transform.LocalTransform() =
				this->transform.GlobalTransform().Value() *
				glm::inverse(this->parent->GlobalTransform().Value());
		} else {
			this->transform.LocalTransform() =
				this->transform.GlobalTransform().Value();
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

int SceneNode::GetID() const {
	return this->id;
}

std::string SceneNode::GetName() const {
	return this->name;
}
void SceneNode::SetName(const std::string& name) {
	this->name = name;
}

Scene* SceneNode::GetScene() {
	return this->scene;
}

bool SceneNode::IsEnabled() const {
	return this->disabledState == 0;
}

bool SceneNode::EnabledSelf() const {
	return (this->disabledState & 1) == 0;
}
void SceneNode::SetEnabled(bool value) {
	if ((this->disabledState & 1) == value) {
		this->disabledState &= 2;

		GetScene()->SetNodeEnabledInternal(this, value);

		this->disabledState |= !value;
	}
}

const std::vector<SceneNode*> SceneNode::GetChildren() {
	return this->children;
}

SceneNode* SceneNode::GetParent() {
	return this->parent;
}

void SceneNode::SetParent(SceneNode* newParent) {
	GetScene()->ChangeNodeParentInternal(this, newParent);

	if (this->parent) {
		auto posInParentChildren = std::find(
			this->parent->children.begin(), this->parent->children.end(), this);
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
	if (this->GetScene()->GetRootNode() == this) {
		return false;
	}
	if (this->parent == node) {
		return true;
	}

	return this->parent->IsChildOf(node);
}

SceneNode* SceneNode::FindNode(const fs::path& nodePath) const {
	const SceneNode* currentNode = this;

	for (const auto& nodeName : nodePath) {
		auto nodeIter = std::find_if(currentNode->children.begin(),
									 currentNode->children.end(),
									 [&nodeName](SceneNode* child) -> bool {
										 return child->name == nodeName;
									 });

		if (nodeIter == currentNode->children.end()) {
			return nullptr;
		}

		currentNode = *nodeIter;
	};

	return const_cast<SceneNode*>(currentNode);
}

bool SceneNode::TryFindNode(const fs::path& nodePath, SceneNode** node) const {
	SceneNode* result = FindNode(nodePath);

	*node = result;

	return result != nullptr;
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

uint8_t SceneNode::GetLayer() const {
	return this->layer;
}
bool SceneNode::CheckLayerMask(uint32_t layerMask) {
	return ((1 << this->layer) & layerMask) > 0;
}
void SceneNode::SetLayer(uint8_t layer) {
	if (layer >= 0 && layer <= 31) {
		this->layer = layer;
	}
}

const std::vector<GameObject*> SceneNode::AttachedObjects() {
	return this->objects;
}

void SceneNode::DeleteObject(GameObject* obj) {
	this->objects.erase(std::find(this->objects.begin(), this->objects.end(), obj));

	this->scene->DeleteObjectInternal(obj);
}

void SceneNode::operator delete(SceneNode* ptr, std::destroying_delete_t) {
	ptr->GetScene()->QueueDelete(ptr);
}

void SceneNode::Deserialize(const nlohmann::json& data) {
	std::erase(this->objects, nullptr);

	this->scene->nextSceneNodeID = std::max(this->scene->nextSceneNodeID, this->id + 1);

	if (this->parent) {
		spdlog::info("Halleluyah");
	}
}
nlohmann::json SceneNode::Serialize() const {
	nlohmann::json data;

	data["transform"] = Serialization::Serialize(this->transform.localTransform.Value());
	
	return data;
}

Scene* Scene::CreateStandaloneScene() {
	Scene* created = new Scene();

	created->graphics = created->AddComponent<SceneGraphics>();
	created->inputSystem = created->AddComponent<InputSystem>();

	return created;
}

Scene::Scene()
	: root(nullptr),
	  nextSceneNodeID(0),
	  nextGameObjectID(0),
	  graphics(nullptr),
	  inputSystem(nullptr) {
	this->root = CreateNode("root");
}

Scene::~Scene() {
	this->resources.Purge();

	delete this->root;

	for (auto component : this->components) {
		delete component;
	}
}

void Scene::DeleteObjectInternal(GameObject* obj) {
	SceneNode* node = obj->node;

	this->messageTree.RemoveMessageReceiver(obj);

	for (auto* component : this->components) {
		GameObjectSystemBase* componentAsSystem = dynamic_cast<GameObjectSystemBase*>(component);

		if (componentAsSystem) {
			componentAsSystem->UnregisterObjectForced(obj);
		}
	}
}

void Scene::DeleteNodeInternal(SceneNode* node) {
	this->messageTree.RemoveNode(node);

	if (node == this->root) {
		this->root = CreateNode("root");
	}
}

void Scene::SetNodeEnabledInTreeInternal(SceneNode* node, bool enabled) {
	if (enabled) {
		node->disabledState &= 1;
	}
	else {
		node->disabledState |= 2;
	}

	if (!node->EnabledSelf()) {
		return;
	}

	for (auto child : node->children) {
		SetNodeEnabledInTreeInternal(child, enabled);
	}
}

void Scene::DeserializeGameObject(SceneNode* node, json data) {

}

void Scene::SetNodeEnabledInternal(SceneNode* node, bool enabled) {
	if (enabled) {
		this->messageTree.PropagateMessage<Message::OnEnable>(node);
	} else {
		this->messageTree.PropagateMessage<Message::OnDisable>(node);
	}

	for (auto child : node->children) {
		SetNodeEnabledInTreeInternal(child, enabled);
	}
}

void Scene::SetGameObjectEnabledInternal(GameObject* obj, bool enabled) {
	if (enabled) {
		this->messageTree.MessageObject<Message::OnEnable>(obj);
	} else {
		this->messageTree.MessageObject<Message::OnDisable>(obj);
	}
}

void Scene::ChangeNodeParentInternal(SceneNode* node, SceneNode* newParent) {
	if (newParent != nullptr) {
		this->messageTree.MoveNode(node, newParent);
	}
}

void Scene::AddObjectToSystems(GameObject* obj) {
	for (SceneComponent* component : this->components) {
		GameObjectSystemBase* sys = dynamic_cast<GameObjectSystemBase*>(component);

		if (sys && sys->ValidObject(obj)) {
			sys->RegisterObject(obj);
		}
	}
}

SceneNode* Scene::CreateNode() {
	return CreateNode(this->root, "");
}
SceneNode* Scene::CreateNode(SceneNode* parent) {
	return CreateNode(parent, "");
}

SceneNode* Scene::CreateNode(const std::string& name) {
	return CreateNode(this->root, name);
}
SceneNode* Scene::CreateNode(SceneNode* parent, const std::string& name) {
	SceneNode* result = new SceneNode(this);

	result->id = this->nextSceneNodeID;
	result->name = name;
	result->parent = parent ? parent : this->root;

	this->messageTree.AddNode(result);

	if (result->parent) {
		result->SetParent(result->parent);
	} else {
		this->root = result;

		result->parent = nullptr;
	}

	this->nextSceneNodeID += 1;

	return result;
}

ResourceDatabase* Scene::Resources() {
	return &this->resources;
}

InputSystem* Scene::Input() {
	return this->inputSystem;
}

SceneGraphics* Scene::GetGraphics() {
	return this->graphics;
}

SceneNode* Scene::GetRootNode() {
	return this->root;
}

SceneNode* Scene::FindNode(const fs::path& nodePath) const {
	return this->root->FindNode(nodePath);
}

bool Scene::TryFindNode(const fs::path& nodePath, SceneNode** node) const {
	return this->root->TryFindNode(nodePath, node);
}

void Scene::DeleteObject(GameObject* obj) {
	delete obj;
}

void Scene::DeleteNode(SceneNode* node) {
	delete node;
}

void Scene::FlushQueues() {
	while (!this->deletedObjectsQueue.empty() || !this->deletedNodesQueue.empty()) {
		while (!this->deletedObjectsQueue.empty()) {
			auto deleted = this->deletedObjectsQueue.front();
			
			deleted->~GameObject();
			
			std::free(deleted);

			this->deletedObjectsQueue.pop();
		}

		if (!this->deletedNodesQueue.empty()) {
			auto deleted = this->deletedNodesQueue.front();
			deleted->~SceneNode();
			
			this->deletedNodesQueue.pop();
		}
	}
}

void Scene::QueueDelete(SceneNode* node) {
	this->deletedNodesQueue.push(node);

	node->SetEnabled(false);

	for (GameObject* obj : node->objects) {
		delete obj;
	}
}
void Scene::QueueDelete(GameObject* object) {
	this->deletedObjectsQueue.push(object);
	object->SetEnabled(false);
}

void Scene::Update() {
	for (auto& component : this->components) {
		component->OnPreUpdate();
	}

	this->messageTree.PropagateMessage<Message::Update>(this->root);

	for (auto& component : this->components) {
		component->OnPostUpdate();
	}

	this->FlushQueues();
}

void Scene::Render() {
	if (this->GetGraphics() == nullptr) {
		return;
	}

	for (auto& component : this->components) {
		component->OnPreRender();
	}

	this->messageTree.PropagateMessage<Message::Render>(this->root);
	this->messageTree.PropagateMessage<Message::DrawGizmos>(this->root);

	for (auto& component : this->components) {
		component->OnPostRender();
	}
}

void Scene::DrawGizmos() {
	this->messageTree.PropagateMessage<Message::DrawGizmos>(this->root);
}
void Scene::OnEnable() {
	this->messageTree.PropagateMessage<Message::OnEnable>(this->root);
}
void Scene::OnDisable() {
	this->messageTree.PropagateMessage<Message::OnDisable>(this->root);
}

void Scene::DrawImGui() {
	for (auto& component : this->components) {
		component->DrawImGui();
	}
}

void Scene::operator delete(Scene* ptr, std::destroying_delete_t) {
	std::free(ptr);
}

void Scene::Deserialize(const nlohmann::json& json_node) {
	for (int compIndex : json_node["components"]) {
		SceneComponent* comp = (SceneComponent*) Serialization::FetchDeserializedObject(compIndex);
		
		if (!comp) {
			continue;
		}

		comp->scene = this;
		
		this->components.push_back(comp);
	}

	this->graphics = AddComponent<SceneGraphics>();
	this->inputSystem = AddComponent<InputSystem>();

	this->messageTree.AddNode(this->root);
	
	this->messageTree.PropagateMessage<Message::Awake>(this->root);
}

nlohmann::json Scene::Serialize() const {
	json data;

	std::vector<int> componentsData;

	for (SceneComponent* comp : this->components) {
		componentsData.push_back(Serialization::QueueObjectSerialization(comp));
	}
	
	data["components"] = componentsData;
	
	return data;
}
