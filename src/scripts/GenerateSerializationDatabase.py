import clang.cindex as clang
import sys
import os
import os.path
from typing import Self
from enum import Enum
import json
import re as regex
from TypeDatabase import *

serialized_types: dict[str, CppType] = {}
data: dict[str, dict] = None

SOURCE_TYPE_DATABASE = sys.argv[1]
DEST_HEADER_FILE_PATH = os.path.dirname(sys.argv[1]) + "/SerializationDecls.h"
DEST_SOURCE_FILE_PATH = os.path.dirname(sys.argv[1]) + "/SerializationDecls.cpp"


class CodeWriter:
	def __init__(self, path: str):
		self.filename = path
		self.file = None
		self.indent = 0


	def __enter__(self):
		self.file = open(self.filename, "w")
		return self
	

	def __exit__(self, exc_type, exc_value, traceback):
		self.file.close()


	def line(self, ln: str = "") -> None:
		self.file.write("\t" * self.indent)
		self.file.write(ln)
		self.file.write("\n")


	def more_indent(self, amount = 1) -> None:
		self.indent += amount
		self.indent = max(self.indent, 0)
	

	def less_indent(self, amount = 1) -> None:
		self.indent -= amount
		self.indent = max(self.indent, 0)


def sanitize_class_name(cls_name: str) -> str:
	return cls_name.replace(":", "_").replace("<", "_").replace(">", "_").replace(", ", "_")


SIMPLE_TYPES = [
	"char",
	"unsigned char",
	"short",
	"unsigned short",
	"int",
	"unsigned int",
	"bool",
	"float",
	"double",
	"std::basic_string<char>",
	"std::__cxx11::basic_string<char>",
]


INTRINSIC_SERIALIZERS = {
	"glm::vec<3, float>": "glm::vec3",
	"glm::vec<4, float>": "glm::vec4"
}


def is_simple_type(type_name: str) -> bool:
	return type_name in SIMPLE_TYPES


def get_enum_type(type: CppType) -> str:
	return f"uint{8 * type.enum_width}_t"


def is_array_type(type: CppType) -> bool:
	return type.name.startswith("std::vector")


def has_serialization_methods(tp: CppType) -> bool:
	return any([
		method for method in tp.methods if (
			method.name == "Serialize"
			and
			not method.is_virtual
			and
			not method.return_type.is_pointer
			and
			method.return_type.type == "nlohmann::basic_json<>"
			and
			len(method.arguments) == 0
		)
	]) and any([
		method for method in tp.methods if (
			method.name == "Deserialize"
			and
			not method.is_virtual
			and
			method.return_type.type == "void"
			and
			len(method.arguments) == 1
			and
			method.arguments[0].type == "nlohmann::basic_json<>"
		)
	])


def is_serialized_type(tp: CppType) -> bool:
	if any([field for field in tp.fields if "__serialized__" in field.attributes]):
		return True
	
	for base in tp.base_classes:
		if is_serialized_type(CppType.get_type(base)):
			return True
	
	return has_serialization_methods(tp)


def write_field_serializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	if is_simple_type(field.type):
		writer.line(lhs + f" = *({field.type} *) (data + {field.offset});")

		return
	
	field_type = CppType.get_type(field.type)

	if field_type.is_enum():
		writer.line(lhs + f" = *({get_enum_type(field_type)} *) (data + {field.offset});")
	elif is_array_type(field_type):
		write_array_serializer(writer, field, lhs)
	# elif sanitize_class_name(field_type.full_name) in INTRINSIC_SERIALIZERS:
	# 	writer.line(lhs + f" = Serialization::Serialize(*({field_type.full_name} *) (data + {field.offset}));")
	# elif not field_type.is_pointer:
	# 	if field.type.full_name in all_classes:
	# 		writer.line(lhs + f" = InternalSerialize{sanitize_class_name(field.type.full_name)}(data + {field.offset});")
	# 	else:
	# 		print(f"WARN: Serializing object of non-serializable type {field.type.full_name}")
	# else:
	# 	if field.type.full_name.removesuffix("*").removesuffix(" ") in all_classes:
	# 		writer.line(lhs + f" = SerializeObject(*(const {field.type.full_name}*) (data + {field.offset}));")
	# 	else:
	# 		print(f"WARN: Serializing object of non-serializable type {field.type.full_name}")


def write_array_serializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	writer.line("{")
	writer.more_indent()

	writer.line(f"json resultArray;")
	writer.line(f"auto& sourceArray = *({field.type} *) (data + {field.offset});")

	writer.line()

	decorated_element_type = CppType.get_type(field.type).template_args[0]
	element_type = CppType.get_type(decorated_element_type.type)

	writer.line("for (auto& element : sourceArray) {")
	writer.more_indent()

	if is_simple_type(element_type):
		writer.line(f"resultArray.push_back(element);")
	else:
		if element_type.is_enum():
			writer.line(f"resultArray.push_back(({get_enum_type(element_type)}) element);")
		elif is_array_type(element_type):
			# write_array_serializer(writer, field, lhs)
			print(f"\tWARN: Nested arrays aren't supported yet: {field.name}")
			pass
		elif element_type.name in INTRINSIC_SERIALIZERS:
			writer.line(f"resultArray.push_back(Serialization::Serialize(element));")
		elif not decorated_element_type.is_pointer:
			if element_type.name in serialized_types:
				writer.line(f"resultArray.push_back(InternalSerialize{sanitize_class_name(element_type.name)}(&element));")
			else:
				print(f"WARN: Serializing array of non-serializable type {element_type.name}")
		else:
			if element_type.name in serialized_types:
				writer.line(f"resultArray.push_back(SerializeObject(element));")
			else:
				print(f"WARN: Serializing array of non-serializable type {element_type.name}")
	
	writer.less_indent()
	writer.line("}")

	writer.line()

	writer.line(f"{lhs} = resultArray;")
	# writer.line(f"// {lhs} = Array field {field.type.full_name} {field.name} at {field.offset}")

	writer.less_indent()
	writer.line("}")


def write_field_deserializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	if is_simple_type(field.type):
		writer.line(f"new(({field.type}*) (raw + {field.offset})) {field.type}{{data[\"{field.name}\"].get<{field.type}>()}};")

		return

	field_type = CppType.get_type(field.type)

	if field_type.is_enum():
		writer.line(f"new(({get_enum_type(field_type)}*) (raw + {field.offset})) {get_enum_type(field_type)}{{data[\"{field.name}\"].get<{get_enum_type(field_type)}>()}};")
	elif is_array_type(field_type):
		write_array_deserializer(writer, field, lhs)
	elif field_type.name in INTRINSIC_SERIALIZERS:
		writer.line(f"new(({field.type}*) (raw + {field.offset})) {field.type}{{Serialization::Deserialize<{field.type}>(data[\"{field.name}\"])}};")
	elif not field.is_pointer:
		if field.type in serialized_types:
			writer.line(f"InternalDeserialize{sanitize_class_name(field.type)}On(reinterpret_cast<volatile {field.type} *>(raw + {field.offset}), data[\"{field.name}\"]);")
		else:
			print(f"WARN: Deserializing object of non-serializable type {field_type.name}")
	else:
		if field.type in serialized_types:
			writer.line(f"*((void**) (raw + {field.offset})) = deserializedObjects[data[\"{field.name}\"].get<int>()];")
		else:
			print(f"WARN: Deserializing object of non-serializable type {field.type}")


def write_array_deserializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	writer.line("{")
	writer.more_indent()

	element_type: CppModifiedType = CppType.get_type(field.type).template_args[0]

	writer.line(f"std::vector<{element_type.type + " *" if element_type.is_pointer else ""}>* dest = new((std::vector<{element_type.type}> *) (raw + {field.offset})) std::vector<{element_type.type + " *" if element_type.is_pointer else ""}>(data[\"{field.name}\"].size());")
	writer.line("int i = 0;")

	writer.line()

	writer.line(f"for (auto& element : data[\"{field.name}\"]) {{")
	writer.more_indent()

	if is_simple_type(element_type.type):
		writer.line(f"dest->push_back(element);")
	else:
		thing_type = CppType.get_type(element_type.type)

		if thing_type.is_enum():
			writer.line(f"dest->push_back(({get_enum_type(thing_type)}) element);")
		elif is_array_type(thing_type):
			# write_array_serializer(writer, field, lhs)
			print(f"\tWARN: Nested arrays aren't supported yet: {field.name}")
			pass
		elif thing_type.name in INTRINSIC_SERIALIZERS:
			writer.line(f"dest->push_back(Serialization::Deserialize<{thing_type.name}>(element));")
		elif not element_type.is_pointer:
			if element_type.type in serialized_types:
				writer.line(f"InternalDeserialize{sanitize_class_name(thing_type.name)}On(&dest->operator[](i), data[\"{field.name}\"][i]);")
			else:
				print(f"WARN: Deserializing array of non-serializable type {thing_type.name}")
		else:
			if element_type.type in serialized_types:
				writer.line(f"dest->push_back(reinterpret_cast<{thing_type.name} *>(deserializedObjects[data[\"{field.name}\"][i].get<int>()]));")
			else:
				print(f"WARN: Deserializing array of non-serializable type {thing_type.name}")

	writer.line()
	writer.line("i++;")
	writer.less_indent()
	writer.line("}")

	writer.less_indent()
	writer.line("}")


def write_serialize_object(writer: CodeWriter, cls_name: str) -> None:
	writer.line(f"int SerializeObject(const {cls_name}* ptr) {{")
	writer.more_indent()

	writer.line("return InternalSerializeObject(ptr, typeid(*ptr));")

	writer.less_indent()
	writer.line("}")


def write_internal_serialize(writer: CodeWriter, type_name: str, tp: CppType) -> None:
	writer.line(f"json InternalSerialize{sanitize_class_name(type_name)}(const void* ptr) {{")
	writer.more_indent()
	
	writer.line(f"spdlog::info(\"Serializing class {type_name}\");")

	writer.line()

	writer.line("json result;")
	writer.line("const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);")

	writer.line()

	for base in tp.base_classes:
		if not is_serialized_type(CppType.get_type(base)):
			continue

		writer.line(f"result.merge_patch(InternalSerialize{sanitize_class_name(base)}(dynamic_cast<const {base} *>(reinterpret_cast<const {type_name} *>(ptr))));")
	
	for field in tp.fields:
		if "__serialized__" not in field.attributes:
			continue

		write_field_serializer(writer, field, f"result[\"{field.name}\"]")

	if has_serialization_methods(tp):
		writer.line(f"result = reinterpret_cast<const {type_name} *>(ptr)->Serialize();")

	writer.line()

	writer.line("return result;")

	writer.less_indent()
	writer.line("}")


def write_internal_deserialize(writer: CodeWriter, type_name: str, tp: CppType) -> None:
	writer.line(f"volatile void* InternalDeserialize{sanitize_class_name(type_name)}On(volatile void* ptr, const json& data) {{")
	writer.more_indent()

	writer.line("volatile uint8_t* raw = reinterpret_cast<volatile uint8_t*>(ptr);")

	writer.line()

	for base in tp.base_classes:
		if not is_serialized_type(CppType.get_type(base)):
			continue

		writer.line(f"InternalDeserialize{sanitize_class_name(base)}On(dynamic_cast<volatile {base} *>(reinterpret_cast<volatile {type_name}*>(ptr)), data);")

	writer.line()	

	if has_serialization_methods(tp):
		writer.line(f"const_cast<{type_name} *>(reinterpret_cast<volatile {type_name} *>(ptr))->Deserialize(data);")
	else:
		for field in tp.fields:
			if "__serialized__" not in field.attributes:
				continue

			write_field_deserializer(writer, field, f"result[\"{field.name}\"]")

	writer.line()

	writer.line("return ptr;")

	writer.less_indent()
	writer.line("}")


def main():
	global serialized_types, data

	print("Generating serialization functions...")
	
	with open(SOURCE_TYPE_DATABASE) as json_file:
		data = json.load(json_file)

		CppType.load_types(data)

	serialized_types = {type_name: tp for type_name, tp in CppType.all_types.items() if tp.access == "public" and is_serialized_type(tp)}

	with CodeWriter(DEST_HEADER_FILE_PATH) as dest_header:
		dest_header.line("#pragma once")
		dest_header.line()

	with CodeWriter(DEST_SOURCE_FILE_PATH) as dest_impl:
		dest_impl.line("#include \"SerializationDecls.h\"")
		dest_impl.line()
		dest_impl.line("#include <unordered_map>")
		dest_impl.line()
		dest_impl.line("#ifdef _WIN32")
		dest_impl.line("#define alloc_aligned(size, align) _aligned_malloc(size, align)")
		dest_impl.line("#else")
		dest_impl.line("#define alloc_aligned(size, align) std::aligned_alloc(align, size)")
		dest_impl.line("#endif")

		dest_impl.line("#include <nlohmann/json.hpp>")
		dest_impl.line()
		dest_impl.line("#include <TypeInfo.h>")

		dest_impl.line()
		dest_impl.line("using json = nlohmann::json;")
		dest_impl.line()

		include_files = []

		for type_name, tp in serialized_types.items():
			if tp.source not in include_files:
				include_files.append(tp.source)
		
		for include_file in include_files:
			dest_impl.line(f"#include \"{include_file}\"")
		
		dest_impl.line()

		dest_impl.line("extern std::vector<json> serializedObjects;")
		dest_impl.line("extern std::vector<void *> deserializedObjects;")

		dest_impl.line()

		dest_impl.line("struct SerializedObject {")
		dest_impl.more_indent()

		dest_impl.line("const void* objPtr;")
		dest_impl.line("int index;")

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line()

		dest_impl.line("std::unordered_map<const void*, SerializedObject> serializationMap;")

		dest_impl.line()

		for type_name in serialized_types:
			dest_impl.line(f"json InternalSerialize{sanitize_class_name(type_name)}(const void* ptr);")
			dest_impl.line(f"int SerializeObject(const {type_name}* ptr);")
			dest_impl.line(f"volatile void* InternalDeserialize{sanitize_class_name(type_name)}On(volatile void* ptr, const json& data);")

		dest_impl.line()

		dest_impl.line("std::unordered_map<std::string, json (*)(const void*)> serializationFunctionLookup {")
		dest_impl.more_indent()

		index = 0
		for type_name in serialized_types:
			dest_impl.line(f"{{ \"{type_name}\", (json (*)(const void*)) InternalSerialize{sanitize_class_name(type_name)} }},")
			index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line()

		dest_impl.line("std::unordered_map<std::string, volatile void* (*)(volatile void*, const json&)> deserializationFunctionLookup {")
		dest_impl.more_indent()

		index = 0
		for type_name in serialized_types:
			dest_impl.line(f"{{ \"{type_name}\", (volatile void* (*)(volatile void*, const json&)) InternalDeserialize{sanitize_class_name(type_name)}On }},")
			index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line()

		dest_impl.line("std::unordered_map<std::string, void* (*)()> constructionFunctionLookup {")
		dest_impl.more_indent()

		index = 0
		for type_name, tp in serialized_types.items():
			if tp.is_abstract():
				continue
			
			if any([constructor for constructor in tp.constructors if len(constructor.arguments) == 0 and constructor.access == "public"]):
				dest_impl.line(f"{{ \"{type_name}\", []() -> void* {{ return new {type_name}(); }} }},")
			elif not tp.is_polymorphic():
				dest_impl.line(f"{{ \"{type_name}\", []() -> void* {{")
				dest_impl.more_indent()

				dest_impl.line(f"void* result = alloc_aligned(sizeof({type_name}), alignof({type_name}));")

				dest_impl.line(f"memset(result, 0, sizeof({type_name}));")

				for field in tp.fields:
					if field.is_pointer:
						dest_impl.line(f"*(((unsigned char**) result) + {field.offset}) = nullptr;")
					elif (field.type in CppType.all_types and CppType.get_type(field.type).default_constructible()) or is_simple_type(field.type):
						dest_impl.line(f"new (({field.type}*) (((unsigned char*) result) + {field.offset})) {field.type}();")
					else:
						dest_impl.line(f"// Field {type_name}.{field.name} cannot be properly initialized, gg")
						print(f"Field {type_name}.{field.name} cannot be properly initialized, gg")

				dest_impl.line("return result;")

				dest_impl.less_indent()
				dest_impl.line(f"}} }},")
			else:
				print(f"WARN: Class {type_name} is polymorphic AND has no public default constructor, and so cannot be deserialized")
			
			index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line()

		dest_impl.line("int InternalSerializeObject(const void* ptr, const std::type_info& objectType) {")
		dest_impl.more_indent()

		dest_impl.line("auto objectAlreadySerializedSearch = serializationMap.find(ptr);")
		dest_impl.line("if (objectAlreadySerializedSearch != serializationMap.end()) {")
		dest_impl.more_indent()

		dest_impl.line("return objectAlreadySerializedSearch->second.index;")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line()

		dest_impl.line("const std::string typeName = TypeInfo::GetTypeInfo(objectType).name;")

		dest_impl.line()

		dest_impl.line("serializedObjects.push_back(json{});")
		
		dest_impl.line("json result;")

		dest_impl.line()

		dest_impl.line("int index = serializedObjects.size() - 1;")

		dest_impl.line("serializationMap[ptr] = { ptr, index };")

		dest_impl.line()

		dest_impl.line(f"result[\"_class_name\"] = typeName;")

		dest_impl.line("result[\"_data\"] = serializationFunctionLookup[typeName](ptr);")

		dest_impl.line()

		dest_impl.line("serializedObjects[index] = result;")

		dest_impl.line()

		dest_impl.line("return index;")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line()

		dest_impl.line("void* InternalConstructObject(const std::string& objectName) {")
		dest_impl.more_indent()

		dest_impl.line("auto objectConstructorSearch = constructionFunctionLookup.find(objectName);")
		dest_impl.line("if (objectConstructorSearch != constructionFunctionLookup.end()) {")
		dest_impl.more_indent()

		dest_impl.line("return objectConstructorSearch->second();")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line("return nullptr;")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line("volatile void* InternalDeserializeJson(volatile void* ptr, const json& data) {")
		dest_impl.more_indent()

		dest_impl.line("const std::string typeName = data[\"_class_name\"];")

		dest_impl.line("auto deserializerSearch = deserializationFunctionLookup.find(typeName);")
		dest_impl.line("if (deserializerSearch != deserializationFunctionLookup.end()) {")
		dest_impl.more_indent()

		dest_impl.line("deserializerSearch->second(ptr, data[\"_data\"]);")

		dest_impl.line("return ptr;")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line()

		dest_impl.line("return nullptr;")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line()

		for type_name, tp in serialized_types.items():
			write_serialize_object(dest_impl, type_name)

			write_internal_serialize(dest_impl, type_name, tp)

			write_internal_deserialize(dest_impl, type_name, tp)

			dest_impl.line()
		
		dest_impl.line("void InternalStartObjectSerialization() {")
		dest_impl.more_indent()

		dest_impl.line("serializationMap.clear();")

		dest_impl.less_indent()
		dest_impl.line("}")
	
	print("\tDone!")


if __name__ == "__main__":
	main()