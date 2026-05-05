import clang.cindex as clang
import sys
import os
import os.path
from typing import Self
from enum import Enum
import json
import re as regex
from TypeDatabase import *

all_classes: dict[str, CppClass] = {}
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


def load_class(class_name, cls_data) -> CppClass:
	if class_name in all_classes:
		return all_classes[class_name]
	else:
		cls = CppClass(cls_data)

		base_classes = []

		if "enclosing_class" in cls_data and cls_data["enclosing_class"] != "":
			cls.enclosing_class = load_class(cls_data["enclosing_class"], data[cls_data["enclosing_class"]])

		for base in cls_data["base_classes"]:
			base_classes.append(load_class(base, data[base]))

		cls.base_classes = base_classes

		all_classes[class_name] = cls


def sanitize_class_name(cls_name: str) -> str:
	return cls_name.replace(":", "_").replace("<", "_").replace(">", "_").replace(", ", "_")


SIMPLE_TYPES = [
	"int",
	"unsigned int",
	"bool",
	"float",
	"double",
	"std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>",
	"std::basic_string<char, std::char_traits<char>, std::allocator<char>>"
]


INTRINSIC_SERIALIZERS = {
	"glm__vec_3_float_glm__defaultp_": "glm::vec3",
	"glm__vec_4_float_glm__defaultp_": "glm::vec4"
}


def is_simple_type(type: CppType) -> bool:
	return type.full_name in SIMPLE_TYPES


def get_enum_type(type: CppType) -> str:
	return f"uint{8 * type.enum_width}_t"


def is_array_type(type: CppType) -> bool:
	return type.full_name.startswith("std::vector")


def is_serialized_class(cls: CppClass) -> bool:
	if any([field for field in cls.fields if "__serialized__" in field.attributes]):
		return True
	
	for base in cls.base_classes:
		if is_serialized_class(base):
			return True
	
	serialization_methods_present = any([
		method for method in cls.methods if (
			method.name == "Serialize"
			and
			not method.is_virtual
			and
			not method.return_type.is_pointer
			and
			method.return_type.name == "basic_json"
			and
			len(method.argument_types) == 0
		)
	]) and any([
		method for method in cls.methods if (
			method.name == "Deserialize"
			and
			not method.is_virtual
			and
			method.return_type.name == "void"
			and
			len(method.argument_types) == 1
			and
			(
				method.argument_types[0].name == "basic_json"
				or
				(method.argument_types[0].is_reference and method.argument_types[0].pointed_type.name == "basic_json")
			)
		)
	])

	return serialization_methods_present


def write_field_serializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	if is_simple_type(field.type):
		writer.line(lhs + f" = *({field.type.full_name} *) (data + {field.offset});")
	elif field.type.is_enum:
		writer.line(lhs + f" = *({get_enum_type(field.type)} *) (data + {field.offset});")
	elif is_array_type(field.type):
		write_array_serializer(writer, field, lhs)
	elif sanitize_class_name(field.type.full_name) in INTRINSIC_SERIALIZERS:
		writer.line(lhs + f" = Serialization::Serialize(*({field.type.full_name} *) (data + {field.offset}));")
	elif not field.type.is_pointer:
		if field.type.full_name in all_classes:
			writer.line(lhs + f" = InternalSerialize{sanitize_class_name(field.type.full_name)}(data + {field.offset});")
		else:
			print(f"WARN: Serializing object of non-serializable type {field.type.full_name}")
	else:
		if field.type.full_name.removesuffix("*").removesuffix(" ") in all_classes:
			writer.line(lhs + f" = SerializeObject(*(const {field.type.full_name}*) (data + {field.offset}));")
		else:
			print(f"WARN: Serializing object of non-serializable type {field.type.full_name}")


def write_array_serializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	writer.line("{")
	writer.more_indent()

	writer.line(f"json resultArray;")
	writer.line(f"auto& sourceArray = *({field.type.full_name} *) (data + {field.offset});")

	writer.line()

	element_type = field.type.template_args[0]

	writer.line("for (auto& element : sourceArray) {")
	writer.more_indent()

	if is_simple_type(element_type):
		writer.line(f"resultArray.push_back(element);")
	elif element_type.is_enum:
		writer.line(f"resultArray.push_back(({get_enum_type(element_type)}) element);")
	elif is_array_type(element_type):
		# write_array_serializer(writer, field, lhs)
		print(f"\tWARN: Nested arrays aren't supported yet: {field.name}")
		pass
	elif sanitize_class_name(element_type.full_name) in INTRINSIC_SERIALIZERS:
		writer.line(f"resultArray.push_back(Serialization::Serialize(element));")
	elif not element_type.is_pointer:
		if element_type.full_name in all_classes:
			writer.line(f"resultArray.push_back(InternalSerialize{sanitize_class_name(element_type.full_name)}(&element));")
		else:
			print(f"WARN: Serializing array of non-serializable type {element_type.full_name}")
	else:
		if element_type.full_name.removesuffix("*").removesuffix(" ") in all_classes:
			writer.line(f"resultArray.push_back(SerializeObject(element));")
		else:
			print(f"WARN: Serializing array of non-serializable type {element_type.full_name}")

	writer.less_indent()
	writer.line("}")

	writer.line()

	writer.line(f"{lhs} = resultArray;")
	# writer.line(f"// {lhs} = Array field {field.type.full_name} {field.name} at {field.offset}")

	writer.less_indent()
	writer.line("}")


def write_field_deserializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	if is_simple_type(field.type):
		writer.line(f"new(({field.type.full_name}*) (raw + {field.offset})) {field.type.full_name}{{data[\"{field.name}\"].get<{field.type.full_name}>()}};")
	elif field.type.is_enum:
		writer.line(f"new(({get_enum_type(field.type)}*) (raw + {field.offset})) {get_enum_type(field.type)}{{data[\"{field.name}\"].get<{get_enum_type(field.type)}>()}};")
	elif is_array_type(field.type):
		write_array_deserializer(writer, field, lhs)
	elif sanitize_class_name(field.type.full_name) in INTRINSIC_SERIALIZERS:
		writer.line(f"new(({field.type.full_name}*) (raw + {field.offset})) {field.type.full_name}{{Serialization::Deserialize<{field.type.full_name}>(data[\"{field.name}\"])}};")
	elif not field.type.is_pointer:
		if field.type.full_name in all_classes:
			writer.line(f"InternalDeserialize{sanitize_class_name(field.type.full_name)}On(reinterpret_cast<volatile {field.type.full_name} *>(raw + {field.offset}), data[\"{field.name}\"]);")
		else:
			print(f"WARN: Deserializing object of non-serializable type {field.type.full_name}")
	else:
		if field.type.full_name.removesuffix("*").removesuffix(" ") in all_classes:
			writer.line(f"*((void**) (raw + {field.offset})) = deserializedObjects[data[\"{field.name}\"].get<int>()];")
		else:
			print(f"WARN: Deserializing object of non-serializable type {field.type.full_name}")


def write_array_deserializer(writer: CodeWriter, field: CppField, lhs: str) -> None:
	writer.line("{")
	writer.more_indent()

	element_type: CppType = field.type.template_args[0]

	writer.line(f"std::vector<{element_type.full_name}>* dest = new((std::vector<{element_type.full_name}> *) (raw + {field.offset})) std::vector<{element_type.full_name}>(data[\"{field.name}\"].size());")
	writer.line("int i = 0;")

	writer.line()

	writer.line(f"for (auto& element : data[\"{field.name}\"]) {{")
	writer.more_indent()

	if is_simple_type(element_type):
		writer.line(f"dest->push_back(element);")
	elif element_type.is_enum:
		writer.line(f"dest->push_back(({get_enum_type(element_type)}) element);")
	elif is_array_type(element_type):
		# write_array_serializer(writer, field, lhs)
		print(f"\tWARN: Nested arrays aren't supported yet: {field.name}")
		pass
	elif sanitize_class_name(element_type.full_name) in INTRINSIC_SERIALIZERS:
		writer.line(f"dest->push_back(Serialization::Deserialize<{element_type.full_name}>(element));")
	elif not element_type.is_pointer:
		if element_type.full_name in all_classes:
			writer.line(f"InternalDeserialize{sanitize_class_name(element_type.full_name)}On(&dest->operator[](i), data[\"{field.name}\"][i]);")
		else:
			print(f"WARN: Deserializing array of non-serializable type {element_type.full_name}")
	else:
		if field.type.full_name.removesuffix("*").removesuffix(" ") in all_classes:
			writer.line(f"dest->push_back(reinterpret_cast<{element_type.full_name} *>(deserializedObjects[data[\"{field.name}\"][i].get<int>()]));")
		else:
			print(f"WARN: Deserializing array of non-serializable type {field.type.full_name}")

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


def write_internal_serialize(writer: CodeWriter, cls_name: str, cls: CppClass) -> None:
	writer.line(f"json InternalSerialize{sanitize_class_name(cls_name)}(const void* ptr) {{")
	writer.more_indent()
	
	writer.line(f"spdlog::info(\"Serializing class {cls_name}\");")

	writer.line()

	writer.line("json result;")
	writer.line("const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);")

	writer.line()

	for base in cls.base_classes:
		if not is_serialized_class(base):
			continue

		writer.line(f"result.merge_patch(InternalSerialize{sanitize_class_name(base.get_full_name())}(dynamic_cast<const {base.name} *>(reinterpret_cast<const {cls_name} *>(ptr))));")
		
	for field in cls.fields:
		if "__serialized__" not in field.attributes:
			continue

		write_field_serializer(writer, field, f"result[\"{field.name}\"]")

	writer.line()

	writer.line("return result;")

	writer.less_indent()
	writer.line("}")


def write_internal_deserialize(writer: CodeWriter, cls_name: str, cls: CppClass) -> None:
	writer.line(f"volatile void* InternalDeserialize{sanitize_class_name(cls_name)}On(volatile void* ptr, const json& data) {{")
	writer.more_indent()

	writer.line("volatile uint8_t* raw = reinterpret_cast<volatile uint8_t*>(ptr);")

	writer.line()

	for base in cls.base_classes:
		if not is_serialized_class(base):
			continue

		writer.line(f"InternalDeserialize{sanitize_class_name(base.get_full_name())}On(dynamic_cast<volatile {base.get_full_name()} *>(reinterpret_cast<volatile {cls_name}*>(ptr)), data);")

	writer.line()	

	for field in cls.fields:
		if "__serialized__" not in field.attributes:
			continue

		write_field_deserializer(writer, field, f"result[\"{field.name}\"]")

	writer.line()

	writer.line("return ptr;")

	writer.less_indent()
	writer.line("}")


def main():
	global all_classes, data

	print("Generating serialization functions...")
	
	with open(SOURCE_TYPE_DATABASE) as json_file:
		data = json.load(json_file)

		for class_name, cls_data in data.items():
			load_class(class_name, cls_data)

	all_classes = {cls_name: cls for cls_name, cls in all_classes.items() if cls.access == "public" and is_serialized_class(cls)}

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

		include_files = []

		for cls_name, cls in all_classes.items():
			if cls.source not in include_files:
				include_files.append(cls.source)
		
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

		for cls_name in all_classes:
			dest_impl.line(f"json InternalSerialize{sanitize_class_name(cls_name)}(const void* ptr);")
			dest_impl.line(f"int SerializeObject(const {cls_name}* ptr);")
			dest_impl.line(f"volatile void* InternalDeserialize{sanitize_class_name(cls_name)}On(volatile void* ptr, const json& data);")

		dest_impl.line()

		dest_impl.line("std::unordered_map<std::string, json (*)(const void*)> serializationFunctionLookup {")
		dest_impl.more_indent()

		index = 0
		for cls_name in all_classes:
			dest_impl.line(f"{{ \"{cls_name}\", (json (*)(const void*)) InternalSerialize{sanitize_class_name(cls_name)} }},")
			index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line()

		dest_impl.line("std::unordered_map<std::string, volatile void* (*)(volatile void*, const json&)> deserializationFunctionLookup {")
		dest_impl.more_indent()

		index = 0
		for cls_name in all_classes:
			dest_impl.line(f"{{ \"{cls_name}\", (volatile void* (*)(volatile void*, const json&)) InternalDeserialize{sanitize_class_name(cls_name)}On }},")
			index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line()

		dest_impl.line("std::unordered_map<std::string, void* (*)()> constructionFunctionLookup {")
		dest_impl.more_indent()

		index = 0
		for cls_name, cls in all_classes.items():
			if cls.is_abstract():
				continue
			
			if cls.is_polymorphic():
				if any([constructor for constructor in cls.constructors if len(constructor.argument_types) == 0 and constructor.access == "public"]):
					dest_impl.line(f"{{ \"{cls_name}\", []() -> void* {{ return new {cls_name}(); }} }},")
				else:
					print(f"WARN: Class {cls_name} is polymorphic AND has no public default constructor, and so cannot be deserialized")
			else:
				dest_impl.line(f"{{ \"{cls_name}\", []() -> void* {{ return alloc_aligned(sizeof({cls_name}), alignof({cls_name})); }} }},")
			
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

		for cls_name, cls in all_classes.items():
			write_serialize_object(dest_impl, cls_name)

			write_internal_serialize(dest_impl, cls_name, cls)

			write_internal_deserialize(dest_impl, cls_name, cls)

			dest_impl.line()
		
		dest_impl.line("void InternalStartObjectSerialization() {")
		dest_impl.more_indent()

		dest_impl.line("serializationMap.clear();")

		dest_impl.less_indent()
		dest_impl.line("}")


	# for serialized_class in all_classes.values():
	# 	serialized_fields = [field for field in serialized_class.get_all_fields() if "__serialized__" in field.attributes]
	# 	serialization_methods = any([
	# 		method for method in serialized_class.methods if (
	# 			method.name == "Serialize"
	# 			and
	# 			not method.is_virtual
	# 			and
	# 			not method.return_type.is_pointer
	# 			and
	# 			method.return_type.name == "basic_json"
	# 			and
	# 			len(method.argument_types) == 0
	# 		)
	# 	]) and any([
	# 		method for method in serialized_class.methods if (
	# 			method.name == "Deserialize"
	# 			and
	# 			not method.is_virtual
	# 			and
	# 			method.return_type.name == "void"
	# 			and
	# 			len(method.argument_types) == 1
	# 			and
	# 			(
	# 				method.argument_types[0].name == "basic_json"
	# 				or
	# 				(method.argument_types[0].is_reference and method.argument_types[0].pointed_type.name == "basic_json")
	# 			)
	# 		)
	# 	])

	# 	if any(serialized_fields) or serialization_methods:
	# 		print(f"Serialized class: {serialized_class.get_full_name()}")

	# 		if serialization_methods:
	# 			print("\tHas Serialize and Deserialize")
			
	# 		print("\tSerialized fields:")
	# 		for f in serialized_fields:
	# 			print(f"\t - {f.name}")
	
	print("\tDone!")


if __name__ == "__main__":
	main()