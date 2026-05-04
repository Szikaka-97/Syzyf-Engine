#include <Serialized.h>
#include <Resources.h>

#include <nlohmann/json.hpp>

std::vector<json> serializedObjects;
std::vector<void *> deserializedObjects;

void InternalStartObjectSerialization();
void* InternalConstructObject(const std::string& objectName);
volatile void* InternalDeserializeJson(volatile void* ptr, const json& data);

void* InternalDeserializeObject(const json& data) {
	deserializedObjects.clear();
	deserializedObjects.reserve(data.size());

	for (const auto& object : data) {
		deserializedObjects.push_back(InternalConstructObject(object["_class_name"]));
	}

	int i = 0;
	for (const auto& object : data) {
		InternalDeserializeJson(deserializedObjects[i], object);

		i++;
	}

	return deserializedObjects[0];
}

void Serialization::StartObjectSerialization() {
	serializedObjects.clear();

	InternalStartObjectSerialization();
}
json Serialization::FinishObjectSerialization() {
	json result;

	for (auto& obj : serializedObjects) {
		result.push_back(obj);
	}

	return result;
}

template<>
glm::vec2 Serialization::Deserialize(const json& json_node) {
	glm::vec2 result;

	result.x = json_node["x"].get<float>();
	result.y = json_node["y"].get<float>();

	return result;
}
template<>
glm::vec3 Serialization::Deserialize(const json& json_node) {
	glm::vec3 result;

	result.x = json_node["x"].get<float>();
	result.y = json_node["y"].get<float>();
	result.z = json_node["z"].get<float>();

	return result;
}
template<>
glm::vec4 Serialization::Deserialize(const json& json_node) {
	glm::vec4 result;

	result.x = json_node["x"].get<float>();
	result.y = json_node["y"].get<float>();
	result.z = json_node["z"].get<float>();
	result.w = json_node["w"].get<float>();

	return result;
}

template<>
glm::ivec2 Serialization::Deserialize(const json& json_node) {
	glm::ivec2 result;

	result.x = json_node["x"].get<int>();
	result.y = json_node["y"].get<int>();

	return result;
}
template<>
glm::ivec3 Serialization::Deserialize(const json& json_node) {
	glm::ivec3 result;

	result.x = json_node["x"].get<int>();
	result.y = json_node["y"].get<int>();
	result.z = json_node["z"].get<int>();

	return result;
}
template<>
glm::ivec4 Serialization::Deserialize(const json& json_node) {
	glm::ivec4 result;

	result.x = json_node["x"].get<int>();
	result.y = json_node["y"].get<int>();
	result.z = json_node["z"].get<int>();
	result.w = json_node["w"].get<int>();

	return result;
}

template<>
glm::mat3 Serialization::Deserialize(const json& json_node) {
	glm::mat3 result;

	result[0][0] = json_node["_00"].get<float>();
	result[0][1] = json_node["_01"].get<float>();
	result[0][2] = json_node["_02"].get<float>();
	result[1][0] = json_node["_10"].get<float>();
	result[1][1] = json_node["_11"].get<float>();
	result[1][2] = json_node["_12"].get<float>();
	result[2][0] = json_node["_20"].get<float>();
	result[2][1] = json_node["_21"].get<float>();
	result[2][2] = json_node["_22"].get<float>();

	return result;
}
template<>
glm::mat4 Serialization::Deserialize(const json& json_node) {
	glm::mat4 result;

	result[0][0] = json_node["_00"].get<float>();
	result[0][1] = json_node["_01"].get<float>();
	result[0][2] = json_node["_02"].get<float>();
	result[0][3] = json_node["_03"].get<float>();
	result[1][0] = json_node["_10"].get<float>();
	result[1][1] = json_node["_11"].get<float>();
	result[1][2] = json_node["_12"].get<float>();
	result[1][3] = json_node["_13"].get<float>();
	result[2][0] = json_node["_20"].get<float>();
	result[2][1] = json_node["_21"].get<float>();
	result[2][2] = json_node["_22"].get<float>();
	result[2][3] = json_node["_23"].get<float>();
	result[3][0] = json_node["_30"].get<float>();
	result[3][1] = json_node["_31"].get<float>();
	result[3][2] = json_node["_32"].get<float>();
	result[3][3] = json_node["_33"].get<float>();

	return result;
}

json Serialization::Serialize(const glm::vec2& v) {
	json result;

	result["x"] = v.x;
	result["y"] = v.y;

	return result;
}
json Serialization::Serialize(const glm::vec3& v) {
	json result;

	result["x"] = v.x;
	result["y"] = v.y;
	result["z"] = v.z;

	return result;
}
json Serialization::Serialize(const glm::vec4& v) {
	json result;

	result["x"] = v.x;
	result["y"] = v.y;
	result["z"] = v.z;
	result["w"] = v.w;

	return result;
}

json Serialization::Serialize(const glm::ivec2& v) {
	json result;

	result["x"] = v.x;
	result["y"] = v.y;

	return result;
}
json Serialization::Serialize(const glm::ivec3& v) {
	json result;

	result["x"] = v.x;
	result["y"] = v.y;
	result["z"] = v.z;

	return result;
}
json Serialization::Serialize(const glm::ivec4& v) {
	json result;

	result["x"] = v.x;
	result["y"] = v.y;
	result["z"] = v.z;
	result["w"] = v.w;

	return result;
}

json Serialization::Serialize(const glm::mat3& v) {
	json result;

	result["_00"] = v[0][0];
	result["_01"] = v[0][1];
	result["_02"] = v[0][2];
	result["_10"] = v[1][0];
	result["_11"] = v[1][1];
	result["_12"] = v[1][2];
	result["_20"] = v[2][0];
	result["_21"] = v[2][1];
	result["_22"] = v[2][2];

	return result;
}
json Serialization::Serialize(const glm::mat4& v) {
	json result;

	result["_00"] = v[0][0];
	result["_01"] = v[0][1];
	result["_02"] = v[0][2];
	result["_03"] = v[0][3];
	result["_10"] = v[1][0];
	result["_11"] = v[1][1];
	result["_12"] = v[1][2];
	result["_13"] = v[1][3];
	result["_20"] = v[2][0];
	result["_21"] = v[2][1];
	result["_22"] = v[2][2];
	result["_23"] = v[2][3];
	result["_30"] = v[3][0];
	result["_31"] = v[3][1];
	result["_32"] = v[3][2];
	result["_33"] = v[3][3];

	return result;
}