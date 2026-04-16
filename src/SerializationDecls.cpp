#include <SerializationDecls.h>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include <GameObject.h>
#include <Texture.h>
#include <Texture.h>
#include <Texture.h>
#include <Shader.h>
#include <Frustum.h>
#include <Frustum.h>
#include <Scene.h>
#include <GameObject.h>
#include <Layer.h>
#include <Material.h>
#include <Light.h>
#include <BoundingBox.h>
#include <Mesh.h>
#include <Camera.h>
#include <Camera.h>
#include <Camera.h>
#include <PostProcessEffect.h>
#include <Tonemapper.h>
#include <ReflectionProbe.h>
#include <MeshRenderer.h>
#include <Bloom.h>
#include <Skybox.h>

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
	
	// Resource
	
	// Texture
	
	// Texture2D
	
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
	
	// Resource
	
	// Texture
	
	// Cubemap
	
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
	
	// Resource
	
	// ShaderProgram
	
	return result;
}

template<>
void DeserializeOn<Plane>(volatile Plane* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Plane
	new((glm::vec3*) (data + 0)) glm::vec3{Serialization::Deserialize<glm::vec3>(json_node["normal"])};
	new((float*) (data + 12)) float{json_node["distance"].get<float>()};
}
template<>
json Serialize<Plane>(const Plane* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Plane";
	json& dataNode = (result["_data"] = json{});
	
	// Plane
	dataNode["normal"] = Serialization::Serialize(*(const glm::vec3 *) (data + 0));
	dataNode["distance"] = *(const float *) (data + 12);
	
	return result;
}

template<>
void DeserializeOn<Frustum>(volatile Frustum* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Frustum
	DeserializeOn<Plane>((Plane*) (data + 0), json_node["top"]["_data"]);
	DeserializeOn<Plane>((Plane*) (data + 16), json_node["bottom"]["_data"]);
	DeserializeOn<Plane>((Plane*) (data + 32), json_node["left"]["_data"]);
	DeserializeOn<Plane>((Plane*) (data + 48), json_node["right"]["_data"]);
	DeserializeOn<Plane>((Plane*) (data + 64), json_node["nearPlane"]["_data"]);
	DeserializeOn<Plane>((Plane*) (data + 80), json_node["farPlane"]["_data"]);
}
template<>
json Serialize<Frustum>(const Frustum* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Frustum";
	json& dataNode = (result["_data"] = json{});
	
	// Frustum
	dataNode["top"] = Serialize<Plane>((const Plane *) (data + 0));
	dataNode["bottom"] = Serialize<Plane>((const Plane *) (data + 16));
	dataNode["left"] = Serialize<Plane>((const Plane *) (data + 32));
	dataNode["right"] = Serialize<Plane>((const Plane *) (data + 48));
	dataNode["nearPlane"] = Serialize<Plane>((const Plane *) (data + 64));
	dataNode["farPlane"] = Serialize<Plane>((const Plane *) (data + 80));
	
	return result;
}

template<>
void DeserializeOn<Scene>(volatile Scene* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Scene
	const_cast<Scene *>(ptr)->Deserialize(json_node["_data"]);
}
template<>
json Serialize<Scene>(const Scene* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Scene";
	json& dataNode = (result["_data"] = json{});
	
	// MessageReceiver
	
	// Scene
	dataNode = const_cast<Scene *>(ptr)->Serialize();
	
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
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	return result;
}

template<>
void DeserializeOn<LayerMask>(volatile LayerMask* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// LayerMask
	new((uint32_t*) (data + 0)) uint32_t{json_node["value"].get<uint32_t>()};
}
template<>
json Serialize<LayerMask>(const LayerMask* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "LayerMask";
	json& dataNode = (result["_data"] = json{});
	
	// LayerMask
	dataNode["value"] = *(const uint32_t *) (data + 0);
	
	return result;
}

template<>
void DeserializeOn<Material>(volatile Material* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// Material
	const_cast<Material *>(ptr)->Deserialize(json_node["_data"]);
}
template<>
json Serialize<Material>(const Material* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Material";
	json& dataNode = (result["_data"] = json{});
	
	// Resource
	
	// Material
	dataNode = const_cast<Material *>(ptr)->Serialize();
	
	return result;
}

template<>
void DeserializeOn<Light>(volatile Light* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
	
	// Light
	new((uint32_t*) (data + 48)) uint32_t{json_node["type"].get<uint32_t>()};
	new((glm::vec3*) (data + 56)) glm::vec3{Serialization::Deserialize<glm::vec3>(json_node["color"])};
	new((float*) (data + 68)) float{json_node["range"].get<float>()};
	new((float*) (data + 72)) float{json_node["spotlightAngle"].get<float>()};
	new((float*) (data + 76)) float{json_node["intensity"].get<float>()};
	new((float*) (data + 80)) float{json_node["linearAttenuation"].get<float>()};
	new((float*) (data + 84)) float{json_node["quadraticAttenuation"].get<float>()};
	new((bool*) (data + 88)) bool{json_node["shadowCasting"].get<bool>()};
}
template<>
json Serialize<Light>(const Light* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Light";
	json& dataNode = (result["_data"] = json{});
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	// ImGuiDrawable
	
	// Light
	dataNode["type"] = *(const uint32_t *) (data + 48);
	dataNode["color"] = Serialization::Serialize(*(const glm::vec3 *) (data + 56));
	dataNode["range"] = *(const float *) (data + 68);
	dataNode["spotlightAngle"] = *(const float *) (data + 72);
	dataNode["intensity"] = *(const float *) (data + 76);
	dataNode["linearAttenuation"] = *(const float *) (data + 80);
	dataNode["quadraticAttenuation"] = *(const float *) (data + 84);
	dataNode["shadowCasting"] = *(const bool *) (data + 88);
	
	return result;
}

template<>
void DeserializeOn<BoundingBox>(volatile BoundingBox* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// BoundingBox
	new((glm::vec3*) (data + 0)) glm::vec3{Serialization::Deserialize<glm::vec3>(json_node["center"])};
	new((glm::vec4*) (data + 12)) glm::vec4{Serialization::Deserialize<glm::vec4>(json_node["axisU"])};
	new((glm::vec4*) (data + 28)) glm::vec4{Serialization::Deserialize<glm::vec4>(json_node["axisV"])};
	new((glm::vec4*) (data + 44)) glm::vec4{Serialization::Deserialize<glm::vec4>(json_node["axisW"])};
}
template<>
json Serialize<BoundingBox>(const BoundingBox* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "BoundingBox";
	json& dataNode = (result["_data"] = json{});
	
	// BoundingBox
	dataNode["center"] = Serialization::Serialize(*(const glm::vec3 *) (data + 0));
	dataNode["axisU"] = Serialization::Serialize(*(const glm::vec4 *) (data + 12));
	dataNode["axisV"] = Serialization::Serialize(*(const glm::vec4 *) (data + 28));
	dataNode["axisW"] = Serialization::Serialize(*(const glm::vec4 *) (data + 44));
	
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
	
	// Resource
	
	// Mesh
	
	return result;
}

template<>
void DeserializeOn<Camera>(volatile Camera* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
	
	// Camera
	new((uint32_t*) (data + 48)) uint32_t{json_node["type"].get<uint32_t>()};
	DeserializeOn<Camera::Perspective>((Camera::Perspective*) (data + 52), json_node["perspectiveData"]["_data"]);
	DeserializeOn<Camera::Orthographic>((Camera::Orthographic*) (data + 68), json_node["orthoData"]["_data"]);
	DeserializeOn<LayerMask>((LayerMask*) (data + 96), json_node["layerMask"]["_data"]);
	new((int*) (data + 100)) int{json_node["priority"].get<int>()};
}
template<>
json Serialize<Camera>(const Camera* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Camera";
	json& dataNode = (result["_data"] = json{});
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	// ImGuiDrawable
	
	// Camera
	dataNode["type"] = *(const uint32_t *) (data + 48);
	dataNode["perspectiveData"] = Serialize<Camera::Perspective>((const Camera::Perspective *) (data + 52));
	dataNode["orthoData"] = Serialize<Camera::Orthographic>((const Camera::Orthographic *) (data + 68));
	dataNode["layerMask"] = Serialize<LayerMask>((const LayerMask *) (data + 96));
	dataNode["priority"] = *(const int *) (data + 100);
	
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
	
	// Camera::Perspective
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
	
	// Camera::Orthographic
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
	
	// Tonemapper
	new((uint32_t*) (data + 48)) uint32_t{json_node["toneOperator"].get<uint32_t>()};
}
template<>
json Serialize<Tonemapper>(const Tonemapper* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Tonemapper";
	json& dataNode = (result["_data"] = json{});
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	// PostProcessEffect
	
	// ImGuiDrawable
	
	// Tonemapper
	dataNode["toneOperator"] = *(const uint32_t *) (data + 48);
	
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
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	// ImGuiDrawable
	
	// ReflectionProbe
	
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
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	// ImGuiDrawable
	
	// MeshRenderer
	dataNode["mesh"] = (*(const Mesh **) (data + 48))->GetName();
	{
		std::vector<json> values;
		for (const auto val : *((const std::vector<Material *> *) (data + 56))) {
			values.push_back(val->GetName());
		}
		
		dataNode["materials"] = values;
	}
	
	return result;
}

template<>
void DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
	
	// Bloom
	new((float*) (data + 88)) float{json_node["threshold"].get<float>()};
	new((float*) (data + 92)) float{json_node["knee"].get<float>()};
	new((float*) (data + 96)) float{json_node["intensity"].get<float>()};
}
template<>
json Serialize<Bloom>(const Bloom* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Bloom";
	json& dataNode = (result["_data"] = json{});
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	// PostProcessEffect
	
	// ImGuiDrawable
	
	// Bloom
	dataNode["threshold"] = *(const float *) (data + 88);
	dataNode["knee"] = *(const float *) (data + 92);
	dataNode["intensity"] = *(const float *) (data + 96);
	
	return result;
}

template<>
void DeserializeOn<Skybox>(volatile Skybox* ptr, const json& json_node) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	// GameObject
	new((int*) (data + 8)) int{json_node["id"].get<int>()};
	
	// Skybox
	*((Material **) (data + 40)) = ResourceDatabase::Global->Get<Material>({json_node["skyMaterial"].get<std::string>()});
}
template<>
json Serialize<Skybox>(const Skybox* ptr) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	json result;
	
	result["_type_name"] = "Skybox";
	json& dataNode = (result["_data"] = json{});
	
	// MessageReceiver
	
	// GameObject
	dataNode["id"] = *(const int *) (data + 8);
	
	// Skybox
	dataNode["skyMaterial"] = (*(const Material **) (data + 40))->GetName();
	
	return result;
}


typedef void (*DeserializeOnSpecialization)(volatile void* ptr, const json& json_node);

void Deserialize(volatile void* ptr, const json& json_node) {
	static const std::unordered_map<std::string, DeserializeOnSpecialization> typeBindings = {
		{ "Texture2D", (DeserializeOnSpecialization) DeserializeOn<Texture2D> },
		{ "Cubemap", (DeserializeOnSpecialization) DeserializeOn<Cubemap> },
		{ "ShaderProgram", (DeserializeOnSpecialization) DeserializeOn<ShaderProgram> },
		{ "Plane", (DeserializeOnSpecialization) DeserializeOn<Plane> },
		{ "Frustum", (DeserializeOnSpecialization) DeserializeOn<Frustum> },
		{ "Scene", (DeserializeOnSpecialization) DeserializeOn<Scene> },
		{ "GameObject", (DeserializeOnSpecialization) DeserializeOn<GameObject> },
		{ "LayerMask", (DeserializeOnSpecialization) DeserializeOn<LayerMask> },
		{ "Material", (DeserializeOnSpecialization) DeserializeOn<Material> },
		{ "Light", (DeserializeOnSpecialization) DeserializeOn<Light> },
		{ "BoundingBox", (DeserializeOnSpecialization) DeserializeOn<BoundingBox> },
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

GameObject* DeserializeGameObject(SceneNode* node, nlohmann::json json_node) {
	std::string className = json_node["_type_name"];
	GameObject* addedObj = nullptr;
	if (className == "GameObject") {
		addedObj = node->AddObject<GameObject>();
	}
	if (className == "Light") {
		addedObj = node->AddObject<Light>();
	}
	if (className == "Camera") {
		addedObj = node->AddObject<Camera>();
	}
	if (className == "Tonemapper") {
		addedObj = node->AddObject<Tonemapper>();
	}
	if (className == "ReflectionProbe") {
		addedObj = node->AddObject<ReflectionProbe>();
	}
	if (className == "MeshRenderer") {
		addedObj = node->AddObject<MeshRenderer>();
	}
	if (className == "Bloom") {
		addedObj = node->AddObject<Bloom>();
	}
	if (className == "Skybox") {
		addedObj = node->AddObject<Skybox>();
	}
	
	Deserialize(addedObj, json_node);
	
	return addedObj;
}

typedef nlohmann::json (*SerializationFunc)(GameObject*);
nlohmann::json SerializeGameObject(GameObject* obj) {
	static const std::unordered_map<std::string, SerializationFunc> typeBindings = {
		{ "Texture2D", (SerializationFunc) Serialize<Texture2D> },
		{ "Cubemap", (SerializationFunc) Serialize<Cubemap> },
		{ "ShaderProgram", (SerializationFunc) Serialize<ShaderProgram> },
		{ "Plane", (SerializationFunc) Serialize<Plane> },
		{ "Frustum", (SerializationFunc) Serialize<Frustum> },
		{ "Scene", (SerializationFunc) Serialize<Scene> },
		{ "GameObject", (SerializationFunc) Serialize<GameObject> },
		{ "LayerMask", (SerializationFunc) Serialize<LayerMask> },
		{ "Material", (SerializationFunc) Serialize<Material> },
		{ "Light", (SerializationFunc) Serialize<Light> },
		{ "BoundingBox", (SerializationFunc) Serialize<BoundingBox> },
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
		{ "Plane", 16 },
		{ "Frustum", 96 },
		{ "Scene", 384 },
		{ "GameObject", 40 },
		{ "LayerMask", 4 },
		{ "Material", 112 },
		{ "Light", 168 },
		{ "BoundingBox", 60 },
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
