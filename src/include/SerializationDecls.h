#pragma once

#include <Serialized.h>

template <typename T>
void DeserializeOn(volatile T* ptr, const json& json_node, std::vector<SerializedReference>& references) = delete;

template <typename T>
json Serialize(const T* ptr);

void Deserialize(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references);

class Scene;

template<>
void DeserializeOn<Scene>(volatile Scene* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<Scene>(const Scene* ptr);


class GameObject;

template<>
void DeserializeOn<GameObject>(volatile GameObject* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<GameObject>(const GameObject* ptr);


class Tonemapper;

template<>
void DeserializeOn<Tonemapper>(volatile Tonemapper* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<Tonemapper>(const Tonemapper* ptr);


class Light;

template<>
void DeserializeOn<Light>(volatile Light* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<Light>(const Light* ptr);


class ReflectionProbe;

template<>
void DeserializeOn<ReflectionProbe>(volatile ReflectionProbe* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<ReflectionProbe>(const ReflectionProbe* ptr);


class MeshRenderer;

template<>
void DeserializeOn<MeshRenderer>(volatile MeshRenderer* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<MeshRenderer>(const MeshRenderer* ptr);


class Camera;

template<>
void DeserializeOn<Camera>(volatile Camera* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<Camera>(const Camera* ptr);


class Bloom;

template<>
void DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<Bloom>(const Bloom* ptr);


class Skybox;

template<>
void DeserializeOn<Skybox>(volatile Skybox* ptr, const json& json_node, std::vector<SerializedReference>& references);

template<>
json Serialize<Skybox>(const Skybox* ptr);

nlohmann::json SerializeGameObject(GameObject* obj);
size_t GetObjectSize(const std::string& className);
