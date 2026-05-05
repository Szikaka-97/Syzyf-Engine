#pragma once

#ifdef __SERIALIZER_RUNNING__
#define serialized __attribute__((annotate("__serialized__")))
#define not_serialized __attribute__((annotate("__not_serialized__")))
#else
#define serialized
#define not_serialized
#endif

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <Resources.h>

using json = nlohmann::json;

class GameObject;
class SceneNode;

class Mesh;

class DoNotSerialize { };

int InternalSerializeObject(const void* ptr, const std::type_info& objectType);
void* InternalDeserializeObject(const json& data);

namespace Serialization {
	void StartObjectSerialization();

	template<typename T>
	int QueueObjectSerialization(const T* obj);

	json FinishObjectSerialization();

	void* FetchDeserializedObject(int index);

	template<typename T>
	json Serialize(const T* obj);

	template<typename T>
	T Deserialize(const json& json_node) = delete;

	template<typename T>
	T* DeserializeObject(const json& json_node);

	template<>
	glm::vec2 Deserialize(const json& json_node);
	template<>
	glm::vec3 Deserialize(const json& json_node);
	template<>
	glm::vec4 Deserialize(const json& json_node);

	template<>
	glm::ivec2 Deserialize(const json& json_node);
	template<>
	glm::ivec3 Deserialize(const json& json_node);
	template<>
	glm::ivec4 Deserialize(const json& json_node);

	template<>
	glm::mat3 Deserialize(const json& json_node);
	template<>
	glm::mat4 Deserialize(const json& json_node);

	json Serialize(const glm::vec2& v);
	json Serialize(const glm::vec3& v);
	json Serialize(const glm::vec4& v);
	
	json Serialize(const glm::ivec2& v);
	json Serialize(const glm::ivec3& v);
	json Serialize(const glm::ivec4& v);
	
	json Serialize(const glm::mat3& v);
	json Serialize(const glm::mat4& v);
};

template <typename T>
int Serialization::QueueObjectSerialization(const T* obj) {
	return InternalSerializeObject(obj, typeid(*obj));
}

template <typename T>
json Serialization::Serialize(const T* obj) {
	StartObjectSerialization();

	QueueObjectSerialization(obj);

	return FinishObjectSerialization();
}

template<typename T>
T* Serialization::DeserializeObject(const json& data) {
	return reinterpret_cast<T*>(InternalDeserializeObject(data));

}