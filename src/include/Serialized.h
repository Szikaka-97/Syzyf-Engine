#pragma once

#include <concepts>
#ifdef __SERIALIZER_RUNNING__
#define serialized __attribute__((annotate("__serialized__")))
#define not_serialized __attribute__((annotate("__not_serialized__")))
#else
#define serialized
#define not_serialized
#endif

#include <nlohmann/json_fwd.hpp>
#include <glm/glm.hpp>
#include <Resources.h>

using json = nlohmann::json;

class GameObject;
class SceneNode;

class Mesh;

class DoNotSerialize { };

namespace Serialization {
	void StartObjectSerialization();
	void QueueObjectSerialization(const void* obj);
	json FinishObjectSerialization();

	template<typename T>
	T Deserialize(const json& json_node);

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
