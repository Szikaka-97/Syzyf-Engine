#pragma once

#include <string>
#include <typeinfo>

struct TypeInfo {
	std::string name;
	int size;
};

const TypeInfo& GetTypeInfo(const std::string& typeName);
const TypeInfo& GetTypeInfo(const std::type_info& typeInfo);