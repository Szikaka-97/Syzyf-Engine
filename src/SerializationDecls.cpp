#include <SerializationDecls.h>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include <GameObject.h>
#include <Scene.h>

using json = nlohmann::json;

template<>
void DeserializeOn<Scene>(volatile Scene* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Scene
	const_cast<Scene *>(ptr)->Deserialize(json_node, references);
}

template<>
json Serialize<Scene>(const Scene* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Scene";
	result["_data"] = const_cast<Scene *>(ptr)->Serialize();
	
	return result;
}
template<>
void DeserializeOn<GameObject>(volatile GameObject* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
}

template<>
json Serialize<GameObject>(const GameObject* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "GameObject";
	json& dataNode = (result["_data"] = json{});
	dataNode["id"] = *(const int *) (data + 8);
	
	return result;
}
template<>
void DeserializeOn<Tonemapper>(volatile Tonemapper* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
}

template<>
json Serialize<Tonemapper>(const Tonemapper* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Tonemapper";
	json& dataNode = (result["_data"] = json{});
	
	return result;
}
template<>
void DeserializeOn<Light>(volatile Light* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
}

template<>
json Serialize<Light>(const Light* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Light";
	json& dataNode = (result["_data"] = json{});
	
	return result;
}
template<>
void DeserializeOn<ReflectionProbe>(volatile ReflectionProbe* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
}

template<>
json Serialize<ReflectionProbe>(const ReflectionProbe* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "ReflectionProbe";
	json& dataNode = (result["_data"] = json{});
	
	return result;
}
template<>
void DeserializeOn<MeshRenderer>(volatile MeshRenderer* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
	
	// MeshRenderer
}

template<>
json Serialize<MeshRenderer>(const MeshRenderer* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "MeshRenderer";
	json& dataNode = (result["_data"] = json{});
	
	return result;
}
template<>
void DeserializeOn<Camera>(volatile Camera* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
}

template<>
json Serialize<Camera>(const Camera* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Camera";
	json& dataNode = (result["_data"] = json{});
	
	return result;
}
template<>
void DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
}

template<>
json Serialize<Bloom>(const Bloom* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Bloom";
	json& dataNode = (result["_data"] = json{});
	
	return result;
}
template<>
void DeserializeOn<Skybox>(volatile Skybox* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
}

template<>
json Serialize<Skybox>(const Skybox* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Skybox";
	json& dataNode = (result["_data"] = json{});
	
	return result;
}

typedef void (*DeserializeOnSpecialization)(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references);

void Deserialize(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	static const std::unordered_map<std::string, DeserializeOnSpecialization> typeBindings = {
		{ "Scene", (DeserializeOnSpecialization) DeserializeOn<Scene> },
		{ "GameObject", (DeserializeOnSpecialization) DeserializeOn<GameObject> },
		{ "Tonemapper", (DeserializeOnSpecialization) DeserializeOn<Tonemapper> },
		{ "Light", (DeserializeOnSpecialization) DeserializeOn<Light> },
		{ "ReflectionProbe", (DeserializeOnSpecialization) DeserializeOn<ReflectionProbe> },
		{ "MeshRenderer", (DeserializeOnSpecialization) DeserializeOn<MeshRenderer> },
		{ "Camera", (DeserializeOnSpecialization) DeserializeOn<Camera> },
		{ "Bloom", (DeserializeOnSpecialization) DeserializeOn<Bloom> },
		{ "Skybox", (DeserializeOnSpecialization) DeserializeOn<Skybox> },
	};
	
	auto deserializerIterator = typeBindings.find(json_node["_type_name"].get<std::string>());
	
	if (deserializerIterator == typeBindings.end()) {
		return;
	}
	
	deserializerIterator->second(ptr, json_node["_data"], references);
}

typedef nlohmann::json (*SerializationFunc)(GameObject*);
nlohmann::json SerializeGameObject(GameObject* obj) {
	static const std::unordered_map<std::string, SerializationFunc> typeBindings = {
		{ "Scene", (SerializationFunc) Serialize<Scene> },
		{ "GameObject", (SerializationFunc) Serialize<GameObject> },
		{ "Tonemapper", (SerializationFunc) Serialize<Tonemapper> },
		{ "Light", (SerializationFunc) Serialize<Light> },
		{ "ReflectionProbe", (SerializationFunc) Serialize<ReflectionProbe> },
		{ "MeshRenderer", (SerializationFunc) Serialize<MeshRenderer> },
		{ "Camera", (SerializationFunc) Serialize<Camera> },
		{ "Bloom", (SerializationFunc) Serialize<Bloom> },
		{ "Skybox", (SerializationFunc) Serialize<Skybox> },
	};
	
	std::string className = obj->GetName();
	
	auto serializerIterator = typeBindings.find(className);
	
	if (serializerIterator == typeBindings.end()) {
		return 0;
	}
	
	return serializerIterator->second(obj);
}

size_t GetObjectSize(const std::string& className) {
	static const std::unordered_map<std::string, size_t> typeBindings = {
		{ "Scene", 384 },
		{ "GameObject", 40 },
		{ "Tonemapper", 80 },
		{ "Light", 168 },
		{ "ReflectionProbe", 80 },
		{ "MeshRenderer", 72 },
		{ "Camera", 104 },
		{ "Bloom", 104 },
		{ "Skybox", 48 },
	};
	
	auto deserializerIterator = typeBindings.find(className);
	
	if (deserializerIterator == typeBindings.end()) {
		return 0;
	}
	
	return deserializerIterator->second;
}
