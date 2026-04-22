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

json_arg_regex = regex.compile(r"(const)?\s+(nlohmann::)?json(\s+&)?")
json_return_regex = regex.compile(r"(nlohmann::)?json")
vector_type_regex = regex.compile(r"(std::)?vector<(.*)>")

#region Serious

class DeserializationStrategy(Enum):
	SIMPLE = 1,
	CHAIN = 2,
	SPECIAL = 3,
	INTRINSIC = 4,


class SerializedClass:
	all_classes: dict[str, Self] = {}

	def __init__(self, class_cursor: clang.Cursor, overlying_class: Self = None):
		if class_cursor.kind != clang.CursorKind.CLASS_DECL and class_cursor.kind != clang.CursorKind.STRUCT_DECL:
			raise RuntimeError("Provided cursor is not a class definition cursor")

		self.cursor = class_cursor
		self.name: str = ((overlying_class.name + "::") if overlying_class else "") + class_cursor.spelling
		self.overlying_class = overlying_class
		self.fields: list[SerializedField] = []
		self.serialization_methods = False
		self.needs_including = False

		SerializedClass.all_classes[self.name] = self

		self.is_abstract = False

		class_part: clang.Cursor

		deserialize_present = False
		serialize_present = False

		self.parent_classes: list[SerializedClass] = []

		for class_part in class_cursor.get_children():
			if class_part.kind == clang.CursorKind.CXX_METHOD and class_part.spelling == "Deserialize":
				method_args = [arg.type.spelling for arg in class_part.get_arguments()]

				deserialize_present = deserialize_present or (
					class_part.result_type.spelling == "void"
					and
					len(method_args) == 1
					and
					json_arg_regex.match(method_args[0])
				)
			elif class_part.kind == clang.CursorKind.CXX_METHOD and class_part.spelling == "Serialize":
				method_args = [arg.type.spelling for arg in class_part.get_arguments()]

				serialize_present = serialize_present or (
					json_return_regex.match(class_part.result_type.spelling)
					and
					len(method_args) == 0
				)

			if class_part.kind == clang.CursorKind.CXX_METHOD and class_part.is_pure_virtual_method():
				self.is_abstract = True

			if class_part.kind == clang.CursorKind.STRUCT_DECL or class_part.kind == clang.CursorKind.CLASS_DECL:
				SerializedClass(class_part, self)

				self.needs_including = True
		
		if serialize_present and deserialize_present:
			self.serialization_methods = True
		

		for class_part in self.cursor.get_children():
			if class_part.kind == clang.CursorKind.CXX_BASE_SPECIFIER:
				if class_part.spelling in SerializedClass.all_classes:
					self.parent_classes.append(SerializedClass.all_classes[class_part.spelling])


	def read_fields(self):
		def is_serialized_field(field_cursor: clang.Cursor):
			for field_token in field_cursor.get_children():
				if field_token.kind == clang.CursorKind.ANNOTATE_ATTR:
					return field_token.displayname == "__serialized__"
			
			return False

		for field_decl in self.cursor.type.get_fields():
			if is_serialized_field(field_decl):
				field = SerializedField.get_field(field_decl)

				if field:
					self.fields.append(field)
		

	def instance_of(self, parent_name: str) -> bool:
		if self.name == parent_name:
			return True
		
		for parent in self.parent_classes:
			if parent.instance_of(parent_name):
				return True
			
		return False

	def serialized(self) -> bool:
		if len(self.fields) > 0 or self.serialization_methods:
			return True

		for parent in self.parent_classes:
			if parent.serialized() or parent.is_resource():
				return True

		return False


	def is_resource(self) -> bool:
		return self.instance_of("Resource")


	def __str__(self):
		result = "class " + self.name + " {\n"
		
		for field in self.fields:
			result += "\t" + str(field) + "\n"
		
		result += "}"

		return result


class SpecialSerializers:
	@staticmethod
	def deserialize_GameObjectPtr():
		pass

	@staticmethod
	def serialize_GameObjectPtr():
		pass


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

	INTRISICED_TYPES = {
		"glm::vec2": "glm::vec2",
		"vec2": "glm::vec2",
		"glm::vec3": "glm::vec3",
		"vec3": "glm::vec3",
		"glm::vec4": "glm::vec4",
		"vec4": "glm::vec4",
		"glm::ivec2": "glm::ivec2",
		"ivec2": "glm::ivec2",
		"glm::ivec3": "glm::ivec3",
		"ivec3": "glm::ivec3",
		"glm::ivec4": "glm::ivec4",
		"ivec4": "glm::ivec4",
		"glm::uvec2": "glm::uvec2",
		"uvec2": "glm::uvec2",
		"glm::uvec3": "glm::uvec3",
		"uvec3": "glm::uvec3",
		"glm::uvec4": "glm::uvec4",
		"uvec4": "glm::uvec4",
		"glm::mat3": "glm::mat3",
		"mat3": "glm::mat3",
		"glm::mat4": "glm::mat4",
		"mat4": "glm::mat4",
	}

	SPECIAL_TYPES = {
		"GameObject *" : -1
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

		if field_cursor.type.get_declaration().kind == clang.CursorKind.ENUM_DECL:
			size: int = field_cursor.type.get_size()

			if size == 1:
				return SerializedType(type_name, ("uint8_t", DeserializationStrategy.SIMPLE))
			if size == 2:
				return SerializedType(type_name, ("uint16_t", DeserializationStrategy.SIMPLE))
			if size == 4:
				return SerializedType(type_name, ("uint32_t", DeserializationStrategy.SIMPLE))
			if size == 8:
				return SerializedType(type_name, ("uint64_t", DeserializationStrategy.SIMPLE))


		thing_type: clang.Type = field_cursor.type

		if thing_type.get_declaration().semantic_parent and not (thing_type.get_declaration().semantic_parent.spelling + "::") in type_name and thing_type.get_declaration().semantic_parent.kind != clang.CursorKind.TRANSLATION_UNIT:
			type_name = thing_type.get_declaration().semantic_parent.spelling + "::" + type_name

		if type_name in SerializedType.BUILTIN_SIMPLE_TYPES:
			return SerializedType(type_name, SerializedType.BUILTIN_SIMPLE_TYPES[type_name])

		if type_name in SerializedType.INTRISICED_TYPES:
			return SerializedType(type_name, (SerializedType.INTRISICED_TYPES[type_name], DeserializationStrategy.INTRINSIC))

		# if type_name.endswith("*"):
		# 	type_name = type_name[:-2]

		# 	if type_name in SerializedType.BUILTIN_SIMPLE_TYPES:
		# 		return SerializedType(type_name, (SerializedType.BUILTIN_SIMPLE_TYPES[type_name][0], DeserializationStrategy.POINTER_SIMPLE))
		# 	elif type_name in SerializedClass.all_classes:
		# 		return SerializedType(type_name, (type_name, DeserializationStrategy.POINTER_CLASS))
		# 	else:
		# 		return None

		if "vector<" in type_name:
			return SerializedVector.get_vector_type(field_cursor)
		
		if "map<" in type_name or "unordered_map<" in type_name:
			template_arg_name = field_cursor.type.get_template_argument_type(1).spelling

			if template_arg_name in SerializedType.BUILTIN_SIMPLE_TYPES:
				return SerializedType(type_name, (type_name if type_name.startswith("std::") else "std::" + type_name, DeserializationStrategy.SIMPLE))
			elif template_arg_name in SerializedClass.all_classes:
				return SerializedType(type_name, (type_name, DeserializationStrategy.DICT_VALUE_CHAIN))
			else:
				return None
		
		if type_name in SerializedClass.all_classes:
			type_class = SerializedClass.all_classes[type_name]

			return SerializedType(type_name, (type_name, DeserializationStrategy.CHAIN))
		elif type_name.rstrip("* ") in SerializedClass.all_classes:
			class_name = type_name.rstrip("* ")

			return SerializedType(class_name, (type_name, DeserializationStrategy.SPECIAL))

		return None


class SerializedVector(SerializedType):
	def __init__(self, field_cursor: clang.Cursor):
		super().__init__(field_cursor, (field_cursor.type.spelling, DeserializationStrategy.SPECIAL))
		
		template_arg_name = field_cursor.type.get_template_argument_type(0).spelling

		if template_arg_name in SerializedType.BUILTIN_SIMPLE_TYPES:
			self.element_type = SerializedType(template_arg_name, (template_arg_name if template_arg_name.startswith("std::") else "std::" + template_arg_name, DeserializationStrategy.SIMPLE))
		elif template_arg_name in SerializedClass.all_classes:
			self.element_type = SerializedType(template_arg_name, (template_arg_name, DeserializationStrategy.CHAIN))
		elif template_arg_name.rstrip("* ") in SerializedClass.all_classes:
			class_name = template_arg_name.rstrip("* ")

			self.element_type = SerializedType(class_name, (template_arg_name, DeserializationStrategy.SPECIAL))
		elif template_arg_name in SerializedType.SPECIAL_TYPES:
			self.element_type = SerializedType(template_arg_name, (template_arg_name, SerializedType.SPECIAL))
		

	@classmethod
	def get_vector_type(cls: Self, field_cursor: clang.Cursor) -> Self:
		return SerializedVector(field_cursor)


class SerializedMap(SerializedType):
	def __init__(self, key_type: SerializedType, value_type: SerializedType):
		self.key_type = key_type
		self.value_type = value_type


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


def generate_deserializer_for_field(writer: CodeWriter, field: SerializedField) -> None:
	if field.type.strategy == DeserializationStrategy.SIMPLE:
		writer.line(f"new(({field.type.c_name}*) (data + {field.offset:.0f})) {field.type.c_name}{{json_node[\"{field.name}\"].get<{field.type.c_name}>()}};")
	elif field.type.strategy == DeserializationStrategy.CHAIN:
		writer.line(f"DeserializeOn<{field.type.c_name}>(({field.type.c_name}*) (data + {field.offset:.0f}), json_node[\"{field.name}\"][\"_data\"]);")
	elif field.type.strategy == DeserializationStrategy.SPECIAL:
		if field.type.name in SerializedClass.all_classes:
			field_class = SerializedClass.all_classes[field.type.name]

			if field_class.is_resource():
				writer.line(f"*(({field.type.c_name}*) (data + {field.offset:.0f})) = ResourceDatabase::Global->Get<{field.type.name}>({{json_node[\"{field.name}\"].get<std::string>()}});")
	elif field.type.strategy == DeserializationStrategy.INTRINSIC:
		writer.line(f"new(({field.type.c_name}*) (data + {field.offset:.0f})) {field.type.c_name}{{Serialization::Deserialize<{field.type.c_name}>(json_node[\"{field.name}\"])}};")


def generate_serializer_for_field(writer: CodeWriter, field: SerializedField) -> None:
	if field.type.strategy == DeserializationStrategy.SIMPLE:
		writer.line(f"dataNode[\"{field.name}\"] = *(const {field.type.c_name} *) (data + {field.offset:.0f});")
	elif field.type.strategy == DeserializationStrategy.CHAIN:
		writer.line(f"dataNode[\"{field.name}\"] = Serialization::Serialize<{field.type.c_name}>((const {field.type.c_name} *) (data + {field.offset:.0f}));")

	elif field.type.strategy == DeserializationStrategy.SPECIAL:
		if isinstance(field.type, SerializedVector):
			field_class = SerializedClass.all_classes[field.type.element_type.name]

			writer.line("{")
			writer.more_indent()
			
			writer.line(f"std::vector<json> values;")

			writer.line(f"for (const auto val : *((const {field.type.c_name} *) (data + {field.offset:.0f}))) {{")
			writer.more_indent()

			if field_class.is_resource():
				writer.line("values.push_back((intptr_t) val);")

				writer.line(f"Serialization::QueueSerializeResource<{field.type.element_type.name}>(val);")
			else:
				writer.line("values.push_back(Serialize(val));")

			writer.less_indent()
			writer.line("}")

			writer.line()
			writer.line(f"dataNode[\"{field.name}\"] = values;")

			writer.less_indent()
			writer.line("}")
		elif field.type.name in SerializedClass.all_classes:
			field_class = SerializedClass.all_classes[field.type.name]

			if field_class.is_resource():
				writer.line(f"dataNode[\"{field.name}\"] = (intptr_t) (data + {field.offset:.0f});")
				
				writer.line(f"Serialization::QueueSerializeResource<{field.type.name}>(*(const {field.type.c_name}*) (data + {field.offset:.0f}));")
	elif field.type.strategy == DeserializationStrategy.INTRINSIC:
		writer.line(f"dataNode[\"{field.name}\"] = Serialization::Serialize(*(const {field.type.c_name} *) (data + {field.offset:.0f}));")


def flatten_parents_list(arr: list[SerializedClass]) -> list:
	if arr == []:
		return arr

	visited_states = []
	states_to_visit = arr

	while len(states_to_visit):
		state = states_to_visit.pop()
		
		if state in visited_states:
			continue
		
		visited_states.append(state)
		states_to_visit += state.parent_classes
	
	return visited_states


def generate_deserializer_for_class(writer: CodeWriter, cls: SerializedClass) -> None:
	for parent in cls.parent_classes:
		generate_deserializer_for_class(writer, parent)

	if not cls.serialization_methods and len(cls.fields) == 0:
		return

	writer.line()
	writer.line(f"// {cls.name}")

	if cls.serialization_methods:
		writer.line(f"const_cast<{cls.name} *>(ptr)->Deserialize(json_node[\"_data\"]);")
	else:
		for field in cls.fields:
			generate_deserializer_for_field(writer, field)


def generate_serializer_for_class(writer: CodeWriter, cls: SerializedClass) -> None:
	for parent in cls.parent_classes:
		generate_serializer_for_class(writer, parent)

	writer.line()
	writer.line(f"// {cls.name}")

	if cls.serialization_methods:
		writer.line(f"dataNode = const_cast<{cls.name} *>(ptr)->Serialize();")

		return

	for field in cls.fields:
		generate_serializer_for_field(writer, field)


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
	
	for class_def in class_defs:
		if not class_def.is_anonymous() and class_def.spelling not in SerializedClass.all_classes:
			serialized_class = SerializedClass(class_def)

	classes: list[SerializedClass] = SerializedClass.all_classes.values()

	for cls in classes:
		cls.read_fields()
	
	classes = [cls for cls in classes if cls.serialized()]

	with CodeWriter(DEST_INCLUDE_FILE_PATH) as dest_header:
		dest_header.line("#pragma once")
		dest_header.line()
		dest_header.line("#include <Serialized.h>")

		for cls in classes:
			if cls.serialized()and not cls.is_abstract:
				dest_header.line()

				if cls.needs_including:
					dest_header.line(f"#include <{cls.cursor.location.file.name.split("/include/")[-1]}>")
				elif cls.overlying_class == None:
					dest_header.line(f"class {cls.name};")

				dest_header.line()
				dest_header.line("template<>")
				dest_header.line(f"void Serialization::DeserializeOn<{cls.name}>(volatile {cls.name}* ptr, const json& json_node);")
				dest_header.line()
				dest_header.line("template<>")
				dest_header.line(f"json Serialization::Serialize<{cls.name}>(const {cls.name}* ptr);")
				dest_header.line()


	with CodeWriter(DEST_SOURCE_FILE_PATH) as dest_impl:
		dest_impl.line(f"#include <{sys.argv[3]}.h>")
		dest_impl.line("#include <string>")
		dest_impl.line("#include <unordered_map>")
		dest_impl.line("#include <nlohmann/json.hpp>")
		dest_impl.line()
		dest_impl.line(f"#include <GameObject.h>")
		for cls in classes:
			dest_impl.line(f"#include <{cls.cursor.location.file.name.split("/include/")[-1]}>")
		dest_impl.line()

		dest_impl.line("using json = nlohmann::json;")
		dest_impl.line()

		for cls in classes:
			if cls.serialized() and not cls.is_abstract:
				dest_impl.line("template<>")
				dest_impl.line(f"void Serialization::DeserializeOn<{cls.name}>(volatile {cls.name}* ptr, const json& json_node) {{")
				dest_impl.more_indent()

				dest_impl.line("volatile uint8_t* data = reinterpret_cast<volatile uint8_t*>(ptr);")
				
				generate_deserializer_for_class(dest_impl, cls)

				dest_impl.indent = 0
				dest_impl.line("}")

				dest_impl.line("template<>")
				dest_impl.line(f"json Serialization::Serialize<{cls.name}>(const {cls.name}* ptr) {{")
				dest_impl.more_indent()
				
				dest_impl.line("const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);")
				dest_impl.line("json result;")
				dest_impl.line()
				dest_impl.line(f"result[\"_type_name\"] = \"{cls.name}\";")

				dest_impl.line("json& dataNode = (result[\"_data\"] = json{});")

				generate_serializer_for_class(dest_impl, cls)
				
				dest_impl.line()

				dest_impl.line("return result;")
	
				dest_impl.indent = 0
				dest_impl.line("}")
				dest_impl.line()
		
		dest_impl.line()
		dest_impl.line("typedef void (*DeserializeOnSpecialization)(volatile void* ptr, const json& json_node);")
		dest_impl.line()
		dest_impl.line("void Serialization::Deserialize(volatile void* ptr, const json& json_node) {")

		dest_impl.more_indent()

		dest_impl.line("static const std::unordered_map<std::string, DeserializeOnSpecialization> typeBindings = {")

		dest_impl.more_indent()

		for name, cls in SerializedClass.all_classes.items():
			if cls.serialized() and not cls.is_abstract:
				dest_impl.line(f"{{ \"{cls.name}\", (DeserializeOnSpecialization) Serialization::DeserializeOn<{cls.name}> }},")

		dest_impl.less_indent()

		dest_impl.line("};")
		dest_impl.line()
		dest_impl.line("auto deserializerIterator = typeBindings.find(json_node[\"_type_name\"].get<std::string>());")
		dest_impl.line()
		dest_impl.line("if (deserializerIterator == typeBindings.end()) {")
		dest_impl.line("	return;")
		dest_impl.line("}")
		dest_impl.line()
		dest_impl.line("deserializerIterator->second(ptr, json_node[\"_data\"]);")

		dest_impl.less_indent()

		dest_impl.line("}")

		dest_impl.line()

		dest_impl.line("GameObject* DeserializeGameObject(SceneNode* node, nlohmann::json json_node) {")
		dest_impl.more_indent()

		dest_impl.line("std::string className = json_node[\"_type_name\"];")
		dest_impl.line("GameObject* addedObj = nullptr;")

		index = 0
		for name, cls in SerializedClass.all_classes.items():
			if cls.instance_of("GameObject") and not cls.is_abstract:
				if index == 0:
					dest_impl.line(f"if (className == \"{cls.name}\") {{")
				else:
					dest_impl.line(f"else if (className == {cls.name}) {{")
				
				dest_impl.more_indent()
				dest_impl.line(f"addedObj = node->AddObject<{cls.name}>();")
				dest_impl.less_indent()
				dest_impl.line("}")

		dest_impl.line()
		dest_impl.line("Serialization::Deserialize(addedObj, json_node);")
		dest_impl.line()
		dest_impl.line("return addedObj;")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line()

		dest_impl.line("typedef nlohmann::json (*SerializationFunc)(GameObject*);")
		dest_impl.line("nlohmann::json Serialization::SerializeGameObject(GameObject* obj) {")
		dest_impl.more_indent()
		
		dest_impl.line("static const std::unordered_map<std::string, SerializationFunc> typeBindings = {")
		dest_impl.more_indent()

		for name, cls in SerializedClass.all_classes.items():
			if cls.serialized() and not cls.is_abstract:
				dest_impl.line(f"{{ \"{cls.name}\", (SerializationFunc) Serialization::Serialize<{cls.name}> }},")

		dest_impl.less_indent()
		dest_impl.line("};")
		dest_impl.line()
		dest_impl.line("std::string className = obj->GetName();")
		dest_impl.line()
		dest_impl.line("auto serializerIterator = typeBindings.find(className);")
		dest_impl.line()
		dest_impl.line("if (serializerIterator == typeBindings.end()) {")
		dest_impl.line("	return 0;")
		dest_impl.line("}")
		dest_impl.line()
		dest_impl.line("return serializerIterator->second(obj);")

		dest_impl.less_indent()
		dest_impl.line("}")

		dest_impl.line()

		dest_impl.line("size_t Serialization::GetObjectSize(const std::string& className) {")

		dest_impl.more_indent()

		dest_impl.line("static const std::unordered_map<std::string, size_t> typeBindings = {")

		dest_impl.more_indent()

		for name, cls in SerializedClass.all_classes.items():
			if cls.serialized() and not cls.is_abstract:
				dest_impl.line(f"{{ \"{cls.name}\", {cls.cursor.type.get_size()} }},")

		dest_impl.less_indent()

		dest_impl.line("};")
		dest_impl.line()
		dest_impl.line("auto deserializerIterator = typeBindings.find(className);")
		dest_impl.line()
		dest_impl.line("if (deserializerIterator == typeBindings.end()) {")
		dest_impl.line("	return 0;")
		dest_impl.line("}")
		dest_impl.line()
		dest_impl.line("return deserializerIterator->second;")

		dest_impl.less_indent()

		dest_impl.line("}")


if __name__ == "__main__":
	main()
