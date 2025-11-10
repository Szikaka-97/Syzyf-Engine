#pragma once

#include <filesystem>
#include <map>
#include <concepts>

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

template<class T, typename... T_Params>
concept Loadable = requires(fs::path p, T_Params... loadParams) {
	{ T::Load(p, loadParams...) } -> std::convertible_to<T*>;
};

namespace Resources {
	template<typename... T>
	struct ResourceLoadParams {};

	template<typename T, typename ... T_Rest>
	struct ResourceLoadParams<T, T_Rest...> {
		T first;
		ResourceLoadParams<T_Rest...> rest;

		ResourceLoadParams(const T& first, const T_Rest& ... rest)
		: first(first),
		rest(rest...) { }
	};
	
	template<typename... T_Params>
	struct ResourceIdentifier {
		fs::path resourcePath;
		ResourceLoadParams<T_Params...> loadParams;

		ResourceIdentifier(fs::path resourcePath, T_Params... loadParams):
		resourcePath(resourcePath),
		loadParams(loadParams...) { }

		bool operator<(const ResourceIdentifier<T_Params...>& other) const {
			if (this->resourcePath < other.resourcePath) {
				return true;
			}
			
			if (this->resourcePath > other.resourcePath) {
				return false;
			}

			unsigned char* thisParamsAsBytes = (unsigned char*) &this->loadParams;
			unsigned char* otherParamsAsBytes = (unsigned char*) &other.loadParams;

			int memcmpResult = memcmp(thisParamsAsBytes, otherParamsAsBytes, sizeof(this->loadParams));

			// If I don't do this the compiler screws me over and it stops working
			if (memcmpResult == INT_MAX) {
				spdlog::info("Something went really wrong. Don't panic. Magic value: {}", memcmpResult);
			}

			return memcmpResult < 0;
		}
	};

	template<class T_Resource, typename... T_Params>
	struct ResourceData {
		static std::map<ResourceIdentifier<T_Params...>, T_Resource*> loadedResources;
	};

	template<class T_Resource, typename... T_Params>
		requires(Loadable<T_Resource, T_Params...>)
	T_Resource* Register(T_Resource* resource, fs::path resourcePath, T_Params... loadParams) {
		return ResourceData<T_Resource, T_Params...>::loadedResources[ResourceIdentifier<T_Params...>(resourcePath, loadParams...)] = resource;
	}

	template<class T_Resource, typename... T_Params>
		requires(Loadable<T_Resource, T_Params...>)
	bool IsLoaded(fs::path resourcePath, T_Params... loadParams) {
		return ResourceData<T_Resource, T_Params...>::loadedResources.contains(ResourceIdentifier<T_Params...>(resourcePath, loadParams...));
	}

	template<class T_Resource, typename... T_Params>
		requires(Loadable<T_Resource, T_Params...>)
	T_Resource* Get(fs::path resourcePath, T_Params... loadParams) {
		if (IsLoaded<T_Resource>(resourcePath, loadParams...)) {
			return ResourceData<T_Resource, T_Params...>::loadedResources[ResourceIdentifier<T_Params...>(resourcePath, loadParams...)];
		}
		
		return Register<T_Resource>(T_Resource::Load(resourcePath, loadParams...), resourcePath, loadParams...);
	}
};

template<class T_Resource, typename... T_Params>
std::map<Resources::ResourceIdentifier<T_Params...>, T_Resource*> Resources::ResourceData<T_Resource, T_Params...>::loadedResources;