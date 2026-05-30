#pragma once

#include <unordered_map>
#include <string>
#include <functional>

class PersistentData {
private:
	static std::vector<std::function<void (void)>> clearFunctions;

	template<typename T>
	struct PersistentRegistry {
		std::unordered_map<std::string, T> map;

		inline PersistentRegistry():
		map() {
			PersistentData::clearFunctions.push_back( [this]() -> void { this->map.clear(); } );
		}
	};
public:
	template<typename T>
	static std::unordered_map<std::string, T>& GetRegistry();

	template<typename T>
	static T Get(const std::string& key);

	template<typename T>
	static void Set(const std::string& key, T value);

	template<typename T>
	static void Clear(const std::string& key);

	static void ClearAll();
};

template<typename T>
std::unordered_map<std::string, T>& PersistentData::GetRegistry() {
	static PersistentRegistry<T> registry;
	return registry.map;
}

template<typename T>
T PersistentData::Get(const std::string& key) {
	auto& map = GetRegistry<T>();

	if (map.contains(key)) {
		return map[key];
	}

	return {};
}

template<typename T>
void PersistentData::Set(const std::string& key, T value) {
	auto& map = GetRegistry<T>();

	map[key] = value;
}

template<typename T>
void PersistentData::Clear(const std::string& key) {
	auto& map = GetRegistry<T>();

	map.erase(key);
}