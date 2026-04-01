#include <SerializationDecls.h>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


typedef void (*DeserializeOnSpecialization)(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references);

void Deserialize(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references) {
	static const std::unordered_map<std::string, DeserializeOnSpecialization> typeBindings = {
	};
	
	auto deserializerIterator = typeBindings.find(json_node["_type_name"].get<std::string>());
	
	if (deserializerIterator == typeBindings.end()) {
		return;
	}
	
	deserializerIterator->second(ptr, json_node["_data"], references);
}

size_t GetObjectSize(const std::string& className) {
	static const std::unordered_map<std::string, size_t> typeBindings = {
	};
	
	auto deserializerIterator = typeBindings.find(className);
	
	if (deserializerIterator == typeBindings.end()) {
		return 0;
	}
	
	return deserializerIterator->second;
}
