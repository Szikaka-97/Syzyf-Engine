#pragma once

#include <string>
#include <typeinfo>

struct TypeInfo {
	std::string name;
	int size;

	static const TypeInfo& GetTypeInfo(const std::string& typeName);
	static const TypeInfo& GetTypeInfo(const std::type_info& typeInfo);
	
	template<typename T>
	static const TypeInfo& GetTypeInfo();
};

template<typename T>
const TypeInfo& TypeInfo::GetTypeInfo() {
	return GetTypeInfo(typeid(T));
}