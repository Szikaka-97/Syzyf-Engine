#pragma once

#include <filesystem>
#include <map>
#include <concepts>
#include <typeinfo>

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

class Resource {
public:
	virtual ~Resource() = default;

	virtual fs::path GetPath() const = 0;
	virtual uint64_t GetHash() const = 0;
};

template<class T, typename... T_Params>
concept Loadable = requires(fs::path p, T_Params... loadParams) {
	{ T::Load(p, loadParams...) } -> std::convertible_to<T*>;
} && std::derived_from<T, Resource>;

class ResourceDatabase {
private:
	struct ResourceInfo {
		const std::type_info* type;
		const Resource* resource;
	};

	struct GenericInfo {
		const std::type_info* type;
		const void* resource;
	};

	std::map<const fs::path, ResourceInfo> loadedResources;
	std::map<const fs::path, ResourceInfo> loadedGenericAssets;
public:
	static ResourceDatabase* const Global;

	template<class T_Resource, typename... T_Params>
		requires(Loadable<T_Resource, T_Params...>)
	T_Resource* Get(const fs::path& resourcePath, T_Params... loadParams);

	template<class T_Resource>
		requires(std::derived_from<T_Resource, Resource> && !Loadable<T_Resource>)
	T_Resource* Get(const fs::path& resourcePath);

	template<class T_Resource>
		requires(!std::derived_from<T_Resource, Resource>)
	T_Resource* Get(const fs::path& resourcePath);

	template <typename T_Resource>
		requires(std::derived_from<T_Resource, Resource>)
	void Register(T_Resource* resource, const fs::path& path);

	template <typename T_Resource>
		requires(!std::derived_from<T_Resource, Resource>)
	void Register(T_Resource* resource, const fs::path& path);

	template <typename T_Resource>
		requires(std::derived_from<T_Resource, Resource>)
	bool IsLoaded(const fs::path& path) const;

	template <typename T_Resource>
		requires(!std::derived_from<T_Resource, Resource>)
	bool IsLoaded(const fs::path& path) const;

	void Free(const fs::path& path);

	void Purge();
};

template<class T_Resource, typename... T_Params>
	requires(Loadable<T_Resource, T_Params...>)
T_Resource* ResourceDatabase::Get(const fs::path& resourcePath, T_Params... loadParams) {
	if (IsLoaded<T_Resource>(resourcePath)) {
        spdlog::debug("[Cache HIT] Returning {} from memory.", resourcePath.string());
		return (T_Resource*) this->loadedResources.at(resourcePath).resource;
	}

    spdlog::debug("[Cache MISS] Loading {} from disk.", resourcePath.string());
	T_Resource* res = T_Resource::Load(resourcePath, loadParams...);

	if (res) {
		Register(res, resourcePath);
	}

	return res;
}

template<class T_Resource>
	requires(std::derived_from<T_Resource, Resource> && !Loadable<T_Resource>)
T_Resource* ResourceDatabase::Get(const fs::path& resourcePath) {
	if (IsLoaded<T_Resource>(resourcePath)) {
		return (T_Resource*) this->loadedResources.at(resourcePath).resource;
	}

	return nullptr;
}

template<class T_Resource>
	requires(!std::derived_from<T_Resource, Resource>)
T_Resource* ResourceDatabase::Get(const fs::path& resourcePath) {
	if (IsLoaded<T_Resource>(resourcePath)) {
		return (T_Resource*) this->loadedGenericAssets.at(resourcePath).resource;
	}

	return nullptr;
}

template <typename T_Resource>
	requires(std::derived_from<T_Resource, Resource>)
void ResourceDatabase::Register(T_Resource* resource, const fs::path& path) {
	if (this->loadedResources.contains(path) && this->loadedResources[path].resource) {
		spdlog::warn("Registering a resource multiple times: {}", path.string().c_str());
	}

	this->loadedResources[path] = ResourceInfo {
		.type = &typeid(T_Resource),
		.resource = resource
	};
}

template <typename T_Resource>
	requires(!std::derived_from<T_Resource, Resource>)
void ResourceDatabase::Register(T_Resource* resource, const fs::path& path) {
	if (this->loadedGenericAssets.contains(path)) {
		spdlog::warn("Registering a resource multiple times: {}", path.string().c_str());
	}

	this->loadedGenericAssets[path] = ResourceInfo {
		.type = &typeid(T_Resource),
		.resource = resource
	};
}

template <typename T_Resource>
	requires(std::derived_from<T_Resource, Resource>)
bool ResourceDatabase::IsLoaded(const fs::path& path) const {
	return this->loadedResources.contains(path) && this->loadedResources.at(path).type == &typeid(T_Resource) && this->loadedResources.at(path).resource;
}

template <typename T_Resource>
	requires(!std::derived_from<T_Resource, Resource>)
bool ResourceDatabase::IsLoaded(const fs::path& path) const {
	return this->loadedGenericAssets.contains(path) && this->loadedGenericAssets.at(path).type == &typeid(T_Resource) && this->loadedGenericAssets.at(path).resource;
}
