#include <SerializationDecls.h>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include <GameObject.h>
#include <Texture.h>
#include <Texture.h>
#include <Texture.h>
#include <Shader.h>
#include <Scene.h>
#include <Material.h>
#include <Mesh.h>

using json = nlohmann::json;

template<>
void DeserializeOn<Texture2D>(volatile Texture2D* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
}

template<>
json Serialize<Texture2D>(const Texture2D* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Texture2D";
	json& dataNode = (result["_data"] = json{});
	
	
	return result;
}
template<>
void DeserializeOn<Cubemap>(volatile Cubemap* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
}

template<>
json Serialize<Cubemap>(const Cubemap* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Cubemap";
	json& dataNode = (result["_data"] = json{});
	
	
	return result;
}
template<>
void DeserializeOn<ShaderProgram>(volatile ShaderProgram* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
}

template<>
json Serialize<ShaderProgram>(const ShaderProgram* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "ShaderProgram";
	json& dataNode = (result["_data"] = json{});
	
	
	return result;
}
template<>
void DeserializeOn<Scene>(volatile Scene* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Scene
	const_cast<Scene *>(ptr)->Deserialize(json_node);
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
void DeserializeOn<GameObject>(volatile GameObject* ptr, const json& json_node) {
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
void DeserializeOn<Material>(volatile Material* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Material
	const_cast<Material *>(ptr)->Deserialize(json_node);
}

template<>
json Serialize<Material>(const Material* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Material";
	result["_data"] = const_cast<Material *>(ptr)->Serialize();
	
	return result;
}
template<>
void DeserializeOn<Light>(volatile Light* ptr, const json& json_node) {
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
void DeserializeOn<Mesh>(volatile Mesh* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
}

template<>
json Serialize<Mesh>(const Mesh* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Mesh";
	json& dataNode = (result["_data"] = json{});
	
	
	return result;
}
template<>
void DeserializeOn<Camera>(volatile Camera* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
	
	// Camera
	DeserializeOn<Camera::Perspective>((Camera::Perspective*) (data + 52), json_node["perspectiveData"]);
	DeserializeOn<Camera::Orthographic>((Camera::Orthographic*) (data + 68), json_node["orthoData"]);
}

template<>
json Serialize<Camera>(const Camera* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Camera";
	json& dataNode = (result["_data"] = json{});
	
	dataNode["perspectiveData"] = Serialize<Camera::Perspective>((const Camera::Perspective *) (data + 52));
	dataNode["orthoData"] = Serialize<Camera::Orthographic>((const Camera::Orthographic *) (data + 68));
	
	return result;
}
template<>
void DeserializeOn<Camera::Perspective>(volatile Camera::Perspective* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Camera::Perspective
	new((float*) (data + 0)) float{json_node["fovyDegrees"].get<float>()};
	new((float*) (data + 4)) float{json_node["aspectRatio"].get<float>()};
	new((float*) (data + 8)) float{json_node["nearPlane"].get<float>()};
	new((float*) (data + 12)) float{json_node["farPlane"].get<float>()};
}

template<>
json Serialize<Camera::Perspective>(const Camera::Perspective* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Camera::Perspective";
	json& dataNode = (result["_data"] = json{});
	
	dataNode["fovyDegrees"] = *(const float *) (data + 0);
	dataNode["aspectRatio"] = *(const float *) (data + 4);
	dataNode["nearPlane"] = *(const float *) (data + 8);
	dataNode["farPlane"] = *(const float *) (data + 12);
	
	return result;
}
template<>
void DeserializeOn<Camera::Orthographic>(volatile Camera::Orthographic* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Camera::Orthographic
	new((float*) (data + 0)) float{json_node["left"].get<float>()};
	new((float*) (data + 4)) float{json_node["right"].get<float>()};
	new((float*) (data + 8)) float{json_node["top"].get<float>()};
	new((float*) (data + 12)) float{json_node["bottom"].get<float>()};
}

template<>
json Serialize<Camera::Orthographic>(const Camera::Orthographic* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Camera::Orthographic";
	json& dataNode = (result["_data"] = json{});
	
	dataNode["left"] = *(const float *) (data + 0);
	dataNode["right"] = *(const float *) (data + 4);
	dataNode["top"] = *(const float *) (data + 8);
	dataNode["bottom"] = *(const float *) (data + 12);
	
	return result;
}
template<>
void DeserializeOn<Tonemapper>(volatile Tonemapper* ptr, const json& json_node) {
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
void DeserializeOn<ReflectionProbe>(volatile ReflectionProbe* ptr, const json& json_node) {
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
void DeserializeOn<MeshRenderer>(volatile MeshRenderer* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
	
	// MeshRenderer
	*((Mesh **) (data + 48)) = ResourceDatabase::Global->Get<Mesh>({json_node["mesh"].get<std::string>()});
}

template<>
json Serialize<MeshRenderer>(const MeshRenderer* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "MeshRenderer";
	json& dataNode = (result["_data"] = json{});
	
	dataNode["mesh"] = (*(const Mesh **) (data + 48))->GetName();
	
	return result;
}
template<>
void DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node) {
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
void DeserializeOn<Skybox>(volatile Skybox* ptr, const json& json_node) {
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

typedef void (*DeserializeOnSpecialization)(volatile void* ptr, const json& json_node);

void Deserialize(volatile void* ptr, const json& json_node) {
	static const std::unordered_map<std::string, DeserializeOnSpecialization> typeBindings = {
		{ "Texture2D", (DeserializeOnSpecialization) DeserializeOn<Texture2D> },
		{ "Cubemap", (DeserializeOnSpecialization) DeserializeOn<Cubemap> },
		{ "ShaderProgram", (DeserializeOnSpecialization) DeserializeOn<ShaderProgram> },
		{ "Scene", (DeserializeOnSpecialization) DeserializeOn<Scene> },
		{ "GameObject", (DeserializeOnSpecialization) DeserializeOn<GameObject> },
		{ "Material", (DeserializeOnSpecialization) DeserializeOn<Material> },
		{ "Light", (DeserializeOnSpecialization) DeserializeOn<Light> },
		{ "Mesh", (DeserializeOnSpecialization) DeserializeOn<Mesh> },
		{ "Camera", (DeserializeOnSpecialization) DeserializeOn<Camera> },
		{ "Camera::Perspective", (DeserializeOnSpecialization) DeserializeOn<Camera::Perspective> },
		{ "Camera::Orthographic", (DeserializeOnSpecialization) DeserializeOn<Camera::Orthographic> },
		{ "Tonemapper", (DeserializeOnSpecialization) DeserializeOn<Tonemapper> },
		{ "ReflectionProbe", (DeserializeOnSpecialization) DeserializeOn<ReflectionProbe> },
		{ "MeshRenderer", (DeserializeOnSpecialization) DeserializeOn<MeshRenderer> },
		{ "Bloom", (DeserializeOnSpecialization) DeserializeOn<Bloom> },
		{ "Skybox", (DeserializeOnSpecialization) DeserializeOn<Skybox> },
	};
	
	auto deserializerIterator = typeBindings.find(json_node["_type_name"].get<std::string>());
	
	if (deserializerIterator == typeBindings.end()) {
		return;
	}
	
	deserializerIterator->second(ptr, json_node["_data"]);
}

typedef nlohmann::json (*SerializationFunc)(GameObject*);
nlohmann::json SerializeGameObject(GameObject* obj) {
	static const std::unordered_map<std::string, SerializationFunc> typeBindings = {
		{ "Texture2D", (SerializationFunc) Serialize<Texture2D> },
		{ "Cubemap", (SerializationFunc) Serialize<Cubemap> },
		{ "ShaderProgram", (SerializationFunc) Serialize<ShaderProgram> },
		{ "Scene", (SerializationFunc) Serialize<Scene> },
		{ "GameObject", (SerializationFunc) Serialize<GameObject> },
		{ "Material", (SerializationFunc) Serialize<Material> },
		{ "Light", (SerializationFunc) Serialize<Light> },
		{ "Mesh", (SerializationFunc) Serialize<Mesh> },
		{ "Camera", (SerializationFunc) Serialize<Camera> },
		{ "Camera::Perspective", (SerializationFunc) Serialize<Camera::Perspective> },
		{ "Camera::Orthographic", (SerializationFunc) Serialize<Camera::Orthographic> },
		{ "Tonemapper", (SerializationFunc) Serialize<Tonemapper> },
		{ "ReflectionProbe", (SerializationFunc) Serialize<ReflectionProbe> },
		{ "MeshRenderer", (SerializationFunc) Serialize<MeshRenderer> },
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
		{ "Texture2D", 112 },
		{ "Cubemap", 120 },
		{ "ShaderProgram", 1000 },
		{ "Scene", 384 },
		{ "GameObject", 40 },
		{ "Material", 112 },
		{ "Light", 168 },
		{ "Mesh", 120 },
		{ "Camera", 104 },
		{ "Camera::Perspective", 16 },
		{ "Camera::Orthographic", 16 },
		{ "Tonemapper", 80 },
		{ "ReflectionProbe", 80 },
		{ "MeshRenderer", 80 },
		{ "Bloom", 104 },
		{ "Skybox", 48 },
	};
	
	auto deserializerIterator = typeBindings.find(className);
	
	if (deserializerIterator == typeBindings.end()) {
		return 0;
	}
	
	return deserializerIterator->second;
}
