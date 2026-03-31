#include <Scene.h>

#include <malloc.h>
#include <algorithm>
#include <stack>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <GameObject.h>
#include <Graphics.h>
#include <InputSystem.h>
#include <Layer.h>
#include <spdlog/spdlog.h>

SceneNode::SceneNode(Scene* scene)
    : scene(scene),
      transform(),
      children(),
      parent(nullptr),
      enabled(true),
      layer(Layer::Default),
      name("") {
    this->transform.parent = this;
}

SceneNode::~SceneNode() {
    int objectsCount = this->objects.size();
    GameObject** objectsCopy =
        (GameObject**)alloca(sizeof(GameObject*) * objectsCount);

    std::copy(this->objects.begin(), this->objects.end(), objectsCopy);

    for (int i = 0; i < objectsCount; i++) {
        delete objectsCopy[i];
    }

    this->SetParent(nullptr);

    int childrenCount = this->children.size();
    SceneNode** childrenCopy =
        (SceneNode**)alloca(sizeof(SceneNode*) * childrenCount);

    std::copy(this->children.begin(), this->children.end(), childrenCopy);

    for (int i = 0; i < childrenCount; i++) {
        delete childrenCopy[i];
    }

    this->scene->DeleteNodeInternal(this);
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
    return this->enabled;
}
void SceneNode::SetEnabled(bool value) {
    if (this->enabled != value) {
        this->enabled = true;

        GetScene()->SetNodeEnabledInternal(this, value);

        this->enabled = value;
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
    this->objects.erase(
        std::find(this->objects.begin(), this->objects.end(), obj));

    this->scene->DeleteObjectInternal(obj);
}

void SceneNode::AttachScene(Scene* scene) {
    this->attachedScenes.push_back(scene);

    GetScene()->AttachSceneToNodeInternal(this, scene);
}

void SceneNode::DetachScene(Scene* scene) {
    std::erase(this->attachedScenes, scene);

    GetScene()->DetachSceneFromNodeInternal(this, scene);
}

std::vector<Scene*> SceneNode::GetAttachedScenes() const {
    return this->attachedScenes;
}

void SceneNode::operator delete(SceneNode* ptr, std::destroying_delete_t) {
    ptr->GetScene()->QueueDelete(ptr);
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

    if (this->root->parent) {
        this->root->parent->DetachScene(this);
    }

    delete this->root;

    for (auto component : this->components) {
        delete component;
    }
}

void Scene::DeleteObjectInternal(GameObject* obj) {
    SceneNode* node = obj->node;

    this->messageTree.RemoveMessageReceiver(obj, node);

    for (auto* component : this->components) {
        GameObjectSystemBase* componentAsSystem =
            dynamic_cast<GameObjectSystemBase*>(component);

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

void Scene::SetNodeEnabledInternal(SceneNode* node, bool enabled) {
    if (enabled) {
        this->messageTree.PropagateMessage<Message::OnEnable>(node);
    } else {
        this->messageTree.PropagateMessage<Message::OnDisable>(node);
    }
}

void Scene::SetGameObjectEnabledInternal(GameObject* obj, bool enabled) {
    if (enabled) {
        this->messageTree.MessageObject<Message::OnEnable>(obj, obj->GetNode());
    } else {
        this->messageTree.MessageObject<Message::OnDisable>(obj,
                                                            obj->GetNode());
    }
}

void Scene::ChangeNodeParentInternal(SceneNode* node, SceneNode* newParent) {
    if (newParent != nullptr) {
        this->messageTree.MoveNode(node, newParent);
    }
}

void Scene::AttachSceneToNodeInternal(SceneNode* node, Scene* scene) {
    node->children.push_back(scene->root);
    scene->root->parent = node;

    if (scene->graphics && scene->graphics != this->graphics) {
        scene->RemoveComponent<SceneGraphics>();
    }

    if (scene->inputSystem && scene->inputSystem != this->inputSystem) {
        scene->RemoveComponent<InputSystem>();
    }

    scene->graphics = this->graphics;
    scene->inputSystem = this->inputSystem;

    this->messageTree.AddMessageReceiver(scene, node);
}

void Scene::DetachSceneFromNodeInternal(SceneNode* node, Scene* scene) {
    std::erase(node->children, scene->root);

    this->messageTree.RemoveMessageReceiver(scene, node);
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
    result->parent = nullptr;

    this->messageTree.AddNode(result);

    if (parent) {
        result->SetParent(parent);
    } else if (this->root) {
        result->SetParent(this->root);
    } else {
        this->root = result;
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

void Scene::QueueDelete(SceneNode* node) {
    this->deletedNodesQueue.push(node);
    node->SetEnabled(false);
}
void Scene::QueueDelete(GameObject* object) {
    this->deletedReceiversQueue.push(object);
    object->SetEnabled(false);
}
void Scene::QueueDelete(Scene* scene) {
    this->deletedReceiversQueue.push(scene);
}

void Scene::Update() {
    for (auto& component : this->components) {
        component->OnPreUpdate();
    }

    this->messageTree.PropagateMessage<Message::Update>(this->root);

    for (auto& component : this->components) {
        component->OnPostUpdate();
    }

    while (!this->deletedReceiversQueue.empty()) {
        auto deleted = this->deletedReceiversQueue.front();
        deleted->~MessageReceiver();
        std::free(deleted);
        this->deletedReceiversQueue.pop();
    }

    while (!this->deletedNodesQueue.empty()) {
        auto deleted = this->deletedNodesQueue.front();
        deleted->~SceneNode();
        std::free(deleted);
        this->deletedNodesQueue.pop();
    }
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
    if (ptr->root->parent) {
        ptr->root->parent->GetScene()->QueueDelete(ptr);
    }
}
