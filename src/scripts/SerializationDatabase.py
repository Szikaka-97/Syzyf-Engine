import clang.cindex as clang
import sys
import os
import os.path
from typing import Self
from enum import Enum
import json
import re as regex

assert(len(sys.argv) == 5)

SOURCE_FILES_DIRECTORY = sys.argv[1]
HEADER_FILES_DIRECTORY = sys.argv[2]
DEST_SOURCE_FILE_PATH = sys.argv[1] + "/" + sys.argv[3] + ".cpp"
DEST_INCLUDE_FILE_PATH = sys.argv[2] + "/" + sys.argv[3] + ".h"
COMMAND_FILE = os.path.abspath(sys.argv[4])

assert(os.path.exists(SOURCE_FILES_DIRECTORY))
assert(os.path.exists(HEADER_FILES_DIRECTORY))
assert(os.path.exists(COMMAND_FILE))

json_type_regex = regex.compile(r"(const)?\s+(nlohmann::)?json(\s+&)?")
references_vector_type_regex = regex.compile(r"(std::)?vector<SerializedReference>\s*&")

#region Serious

class DeserializationStrategy(Enum):
	SIMPLE = 1,
	CHAIN = 2,
	VECTOR_CHAIN = 3,
	DICT_VALUE_CHAIN = 4,
	POINTER_SIMPLE = 5
	POINTER_CLASS = 6


class SerializedClass:
	all_classes: dict[str, Self] = {}

	def __init__(self, class_cursor: clang.Cursor):
		if class_cursor.kind != clang.CursorKind.CLASS_DECL and class_cursor.kind != clang.CursorKind.STRUCT_DECL:
			raise RuntimeError("Provided cursor is not a class definition cursor")

		self.cursor = class_cursor
		self.name: str = class_cursor.spelling
		self.fields: list[SerializedField] = []
		self.serialization_methods = False

		SerializedClass.all_classes[self.name] = self

		self.generate_code = True

		class_part: clang.Cursor
		for class_part in class_cursor.get_children():
			deserialize_present = False
			serialize_present = False

			if class_part.kind == clang.CursorKind.CXX_METHOD and class_part.spelling == "Deserialize":
				method_args = [arg.type.spelling for arg in class_part.get_arguments()]

				deserialize_present = (
					class_part.result_type.spelling == "void"
					and
					len(method_args) == 2
					and
					json_type_regex.match(method_args[0])
					and
					references_vector_type_regex.match(method_args[1])
				)
			elif class_part.kind == clang.CursorKind.CXX_METHOD and class_part.spelling == "Serialize":
				method_args = [arg.type.spelling for arg in class_part.get_arguments()]

				serialize_present = (
					class_part.result_type.spelling == "void"
					and
					len(method_args) == 2
					and
					json_type_regex.match(method_args[0])
					and
					references_vector_type_regex.match(method_args[1])
				)
				

			if class_part.kind == clang.CursorKind.CXX_METHOD and class_part.is_pure_virtual_method():
				self.generate_code = False
				break


	def read_fields(self):
		def is_serialized_field(field_cursor: clang.Cursor):
			for field_token in field_cursor.get_children():
				if field_token.kind == clang.CursorKind.ANNOTATE_ATTR:
					return field_token.displayname == "__serialized__"
			
			return False

		for class_part in self.cursor.get_children():
			if class_part.kind == clang.CursorKind.CXX_BASE_SPECIFIER:
				if class_part.spelling in SerializedClass.all_classes:
					self.fields += SerializedClass.all_classes[class_part.spelling].fields

		for field_decl in self.cursor.type.get_fields():
			if is_serialized_field(field_decl):
				field = SerializedField.get_field(field_decl)

				if field:
					self.fields.append(field)
		
		if len(self.fields) == 0:
			self.generate_code = False


	def serialized(self) -> bool:
		return len(self.fields) > 0

	def __str__(self):
		result = "class " + self.name + " {\n"
		
		for field in self.fields:
			result += "\t" + str(field) + "\n"
		
		result += "}"

		return result


class SerializedType:
	BUILTIN_SIMPLE_TYPES = {
		"int": ("int", DeserializationStrategy.SIMPLE),
		"unsigned int": ("unsigned int", DeserializationStrategy.SIMPLE),
		"short": ("short", DeserializationStrategy.SIMPLE),
		"unsigned short": ("unsigned short", DeserializationStrategy.SIMPLE),
		"int8_t": ("int8_t", DeserializationStrategy.SIMPLE),
		"int16_t": ("int16_t", DeserializationStrategy.SIMPLE),
		"int32_t": ("int32_t", DeserializationStrategy.SIMPLE),
		"int64_t": ("int64_t", DeserializationStrategy.SIMPLE),
		"uint8_t": ("uint8_t", DeserializationStrategy.SIMPLE),
		"uint16_t": ("uint16_t", DeserializationStrategy.SIMPLE),
		"uint32_t": ("uint32_t", DeserializationStrategy.SIMPLE),
		"uint64_t": ("uint64_t", DeserializationStrategy.SIMPLE),
		"float": ("float", DeserializationStrategy.SIMPLE),
		"double": ("double", DeserializationStrategy.SIMPLE),
		"bool": ("bool", DeserializationStrategy.SIMPLE),
		"std::string": ("std::string", DeserializationStrategy.SIMPLE),
		"string": ("std::string", DeserializationStrategy.SIMPLE),
	}
	
	def __init__(self, name: str, type_data: tuple):
		self.name = name
		self.c_name = type_data[0]
		self.strategy = type_data[1]

	
	def __str__(self):
		return self.name


	@classmethod
	def get_type(cls: Self, field_cursor: clang.Cursor) -> Self:
		type_name: str = field_cursor.type.spelling
		if type_name in SerializedType.BUILTIN_SIMPLE_TYPES:
			return SerializedType(type_name, SerializedType.BUILTIN_SIMPLE_TYPES[type_name])

		if type_name.endswith("*"):
			type_name = type_name[:-2]

			if type_name in SerializedType.BUILTIN_SIMPLE_TYPES:
				return SerializedType(type_name, (SerializedType.BUILTIN_SIMPLE_TYPES[type_name][0], DeserializationStrategy.POINTER_SIMPLE))
			elif type_name in SerializedClass.all_classes:
				return SerializedType(type_name, (type_name, DeserializationStrategy.POINTER_CLASS))
			else:
				return None

		if "vector<" in type_name:
			template_arg_name = field_cursor.type.get_template_argument_type(0).spelling

			if template_arg_name in SerializedType.BUILTIN_SIMPLE_TYPES:
				return SerializedType(type_name, (type_name if type_name.startswith("std::") else "std::" + type_name, DeserializationStrategy.SIMPLE))
			elif template_arg_name in SerializedClass.all_classes:
				return SerializedType(type_name, (type_name, DeserializationStrategy.VECTOR_CHAIN))
			else:
				return None
		
		if "map<" in type_name or "unordered_map<" in type_name:
			template_arg_name = field_cursor.type.get_template_argument_type(1).spelling

			if template_arg_name in SerializedType.BUILTIN_SIMPLE_TYPES:
				return SerializedType(type_name, (type_name if type_name.startswith("std::") else "std::" + type_name, DeserializationStrategy.SIMPLE))
			elif template_arg_name in SerializedClass.all_classes:
				return SerializedType(type_name, (type_name, DeserializationStrategy.DICT_VALUE_CHAIN))
			else:
				return None
		
		if type_name in SerializedClass.all_classes:
			return SerializedType(type_name, (type_name, DeserializationStrategy.CHAIN))

		return None


class SerializedField:
	def __init__(self, field_cursor: clang.Cursor):
		self.name: str = field_cursor.spelling
		self.type: SerializedType = SerializedType.get_type(field_cursor)
		self.offset: int = field_cursor.get_field_offsetof() / 8

	
	def __str__(self):
		value_rep = ""
		if isinstance(self.value, str):
			value_rep = "\"" + self.value + "\""
		else:
			value_rep = str(self.value)

		return "T + {:>2.0f} => ".format(self.offset) + str(self.type) + " " + self.name + " = " + value_rep


	@staticmethod
	def get_field(field_cursor: clang.Cursor) -> Self:
		field = SerializedField(field_cursor)

		if field.type:
			return field
		else:
			print(f"WARN: Serialized field {field_cursor.semantic_parent.spelling}.{field.name} is of non serializable type: {field_cursor.type.spelling}.")
			return None

#endregion

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


def get_class_definitions(unit: clang.TranslationUnit) -> list[clang.Cursor]:
	cursor: clang.Cursor = unit.cursor

	definitions = [
		token for token in cursor.get_children() if (
			(token.kind == clang.CursorKind.CLASS_DECL or token.kind == clang.CursorKind.STRUCT_DECL) and token.is_definition()
		)
	]

	return definitions


def construct_file(files: list[str], compile_args: list[str]) -> clang.TranslationUnit:
	compiled_file = ""

	for h_file in files:
		compiled_file += f"#include<{os.path.basename(h_file)}>\n"
	
	result = clang.Index.create().parse(
		f"main.cpp",
		["-D__SERIALIZER_RUNNING__", "-std=c++23", "-I/usr/lib/clang/18.1.3/include"] + compile_args,
		unsaved_files=[(f"main.cpp", compiled_file)]
	)

	diagnostic_messages = [message for message in result.diagnostics if message.severity > 2]

	if len(diagnostic_messages) > 0:
		raise RuntimeError(diagnostic_messages)

	return result


def generate_cpp_for_class(writer: CodeWriter, cls: SerializedClass) -> None:
	writer.line("template<>")
	writer.line(f"void DeserializeOn<{cls.name}>(volatile {cls.name}* ptr, const json& json_node, std::vector<SerializedReference>& references) {{")
	writer.more_indent()
	
	writer.line("volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);")
	writer.line("")

	for field in cls.fields:
		if field.type.strategy == DeserializationStrategy.SIMPLE:
			writer.line(f"new(({field.type.c_name}*) (data + {field.offset:.0f})) {field.type.c_name}{{json_node[\"{field.name}\"].get<{field.type.c_name}>()}};")
		elif field.type.strategy == DeserializationStrategy.CHAIN:
			writer.line(f"DeserializeOn<{field.type.c_name}>(({field.type.c_name}*) (data + {field.offset:.0f}), json_node[\"{field.name}\"]);")
		elif field.type.strategy == DeserializationStrategy.VECTOR_CHAIN:
			writer.line("{")
			writer.more_indent()
			writer.line(f"auto value_{field.name} = new(({field.type.c_name}*) (data + {field.offset:.0f})) {field.type.c_name}{{json_node[\"{field.name}\"].size()}};")
			writer.line(f"for (int i = 0; i < value_{field.name}->size(); i++) {{")
			writer.more_indent()
			writer.line(f"Deserialize(&(*value_{field.name})[i], json_node[\"{field.name}\"][i]);")
			writer.less_indent()
			writer.line("}")
			writer.less_indent()
			writer.line("}")
		elif field.type.strategy == DeserializationStrategy.DICT_VALUE_CHAIN:
			writer.line("{")
			writer.more_indent()
			writer.line(f"auto value_{field.name} = new(({field.type.c_name}*) (data + {field.offset:.0f})) {field.type.c_name}{{}};")
			writer.line(f"auto value__list = json_node[\"{field.name}\"].get<json::object_t>();")
			writer.line("for (auto& child : value__list) {")
			writer.more_indent()
			writer.line(f"Deserialize(&(*value_{field.name})[child.first], child.second, references);")
			writer.less_indent()
			writer.line("}")
			writer.less_indent()
			writer.line("}")
		elif field.type.strategy == DeserializationStrategy.POINTER_SIMPLE:
			writer.line(f"*(({field.type.c_name}**) (data + {field.offset:.0f})) = new {field.type.c_name}(json_node[\"{field.name}\"].get<{field.type.c_name}>());")
		elif field.type.strategy == DeserializationStrategy.POINTER_CLASS:
			writer.line(f"references.push_back({{(void**) (data + {field.offset:.0f}), json_node[\"{field.name}\"][\"_index\"].get<int>()}});")
		# add_line(f"*value_{field.name} = ;")

	writer.indent = 0
	writer.line("}")


def main():
	compile_args: list[str] = []
	
	with open(COMMAND_FILE, "r") as compile_json_file:
		compile_commands: list = json.load(compile_json_file)

		for command in compile_commands:
			if SOURCE_FILES_DIRECTORY in command["file"]:
				consume_next = False

				arg: str
				for arg in command["command"].split(" "):
					if consume_next or arg.startswith("-D") or arg.startswith("-I"):
						compile_args.append(arg)
						consume_next = False
					if arg == "-isystem":
						compile_args.append(arg)
						consume_next = True
					if arg.startswith("@"):
						rsp_path = os.path.abspath(COMMAND_FILE + "/../src/" + arg[1:])

						if os.path.exists(rsp_path):
							with open(rsp_path, "r") as rsp_file:
								rsp_args = rsp_file.readline()
								rsp_args = rsp_args.removesuffix("\n")

								rsp_args_array = rsp_args.split(" ")

								rsp_args_array = [ file_path.removeprefix("\"").removesuffix("\"") for file_path in rsp_args_array ]

								compile_args += rsp_args_array

				break

	files = [os.path.abspath(HEADER_FILES_DIRECTORY + "/" + file) for file in os.listdir(HEADER_FILES_DIRECTORY)]

	class_defs = get_class_definitions(construct_file(files, compile_args))
	
	classes: list[SerializedClass] = []

	for class_def in class_defs:
		if not class_def.is_anonymous() and class_def.spelling not in SerializedClass.all_classes:
			serialized_class = SerializedClass(class_def)
			classes.append(serialized_class)

	for cls in classes:
		cls.read_fields()
	
	classes = [cls for cls in classes if cls.serialized() > 0]

	with CodeWriter(DEST_INCLUDE_FILE_PATH) as dest_header:
		dest_header.line("#pragma once")
		dest_header.line()
		dest_header.line("#include <Serialized.h>")
		dest_header.line()
		dest_header.line("template <typename T>")
		dest_header.line("void DeserializeOn(volatile T* ptr, const json& json_node, std::vector<SerializedReference>& references) = delete;")
		dest_header.line()
		dest_header.line("void Deserialize(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references);")

		for cls in classes:
			dest_header.line()
			dest_header.line(f"class {cls.name};")
			dest_header.line()
			dest_header.line("template<>")
			dest_header.line(f"void DeserializeOn<{cls.name}>(volatile {cls.name}* ptr, const json& json_node, std::vector<SerializedReference>& references);")


	with CodeWriter(DEST_SOURCE_FILE_PATH) as dest_impl:
		dest_impl.line(f"#include <{sys.argv[3]}.h>")
		dest_impl.line("#include <string>")
		dest_impl.line("#include <unordered_map>")
		dest_impl.line("#include <nlohmann/json.hpp>")
		dest_impl.line()
		dest_impl.line("using json = nlohmann::json;")
		dest_impl.line()

		for cls in classes:
			if cls.generate_code:
				generate_cpp_for_class(dest_impl, cls)
		
		dest_impl.line()
		dest_impl.line("typedef void (*DeserializeOnSpecialization)(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references);")
		dest_impl.line()
		dest_impl.line("void Deserialize(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references) {")

		dest_impl.more_indent()

		dest_impl.line("static const std::unordered_map<std::string, DeserializeOnSpecialization> typeBindings = {")

		dest_impl.more_indent()

		for name, cls in SerializedClass.all_classes.items():
			if cls.generate_code:
				dest_impl.line(f"{{ \"{cls.name}\", (DeserializeOnSpecialization) DeserializeOn<{cls.name}> }},")

		dest_impl.less_indent()

		dest_impl.line("};")
		dest_impl.line()
		dest_impl.line("auto deserializerIterator = typeBindings.find(json_node[\"_type_name\"].get<std::string>());")
		dest_impl.line()
		dest_impl.line("if (deserializerIterator == typeBindings.end()) {")
		dest_impl.line("	return;")
		dest_impl.line("}")
		dest_impl.line()
		dest_impl.line("deserializerIterator->second(ptr, json_node[\"_data\"], references);")

		dest_impl.less_indent()

		dest_impl.line("}")


if __name__ == "__main__":
	main()