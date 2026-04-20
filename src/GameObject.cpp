
#include <GameObject.h>

#include <Scene.h>
#include <Light.h>

#include <cxxabi.h>

GameObject::~GameObject() {
	this->node->DeleteObject(this);
}

int GameObject::GetID() const {
	return this->id;
}

std::string GameObject::GetName() const {
    return DemangleTypeName(this->runtimeTypeInfo->name());
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

	GetScene()->SetGameObjectEnabledInternal(this, enabled);

	this->enabled = enabled;
}

void GameObject::operator delete(GameObject* ptr, std::destroying_delete_t) {
	ptr->GetScene()->QueueDelete(ptr);
}

std::string DemangleTypeName(const char* mangledName) {
#if defined(__GNUG__) || defined(__clang__)
    int status = -1;
    std::unique_ptr<char, decltype(&std::free)> demangled{
        abi::__cxa_demangle(mangledName, nullptr, nullptr, &status),
        std::free
    };
    
    return (status == 0 && demangled) ? demangled.get() : mangledName;

#elif defined(_MSC_VER)
    std::string_view sv(mangledName);
    if (sv.starts_with("class ")) {
        sv.remove_prefix(6);
    } else if (sv.starts_with("struct ")) {
        sv.remove_prefix(7);
    }
    return std::string(sv);

#else
    return mangledName;
#endif
}

