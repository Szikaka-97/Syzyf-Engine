#include <SerializationDecls.h>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

template<>
void DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);
	
	new((float*) (data + 88)) float{json_node["threshold"].get<float>()};
	new((float*) (data + 92)) float{json_node["knee"].get<float>()};
	new((float*) (data + 96)) float{json_node["intensity"].get<float>()};
}

typedef void (*DeserializeOnSpecialization)(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references);

void Deserialize(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	static const std::unordered_map<std::string, DeserializeOnSpecialization> typeBindings = {
		{ "Bloom", (DeserializeOnSpecialization) DeserializeOn<Bloom> },
	};
	
	auto deserializerIterator = typeBindings.find(json_node["_type_name"].get<std::string>());
	
	if (deserializerIterator == typeBindings.end()) {
		return;
	}
	
	deserializerIterator->second(ptr, json_node["_data"], references);
}
