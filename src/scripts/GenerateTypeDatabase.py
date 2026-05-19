import json
import sys
import clang.cindex as clang # type: ignore
from typing import Self
from enum import Enum
import os
from os import path
import traceback

clang.TemplateArgumentKind.STRUCTURAL_VALUE = clang.TemplateArgumentKind(5)

SOURCE_FILES_DIRECTORY = sys.argv[1]
HEADER_FILES_DIRECTORY = sys.argv[2]
COMMAND_FILE = path.abspath(sys.argv[3])


class CppField:
	def __init__(self, field_cursor: clang.Cursor):
		self.cursor = field_cursor
		
		self.name = field_cursor.spelling

		self.type = CppType.read_type(field_cursor.type.get_canonical())

		self.is_pointer = field_cursor.type.kind == clang.TypeKind.POINTER
		self.is_reference = field_cursor.type.kind == clang.TypeKind.LVALUEREFERENCE
		self.is_const = field_cursor.type.is_const_qualified

		self.attributes = []

		self.access = str_access_specifier(field_cursor.access_specifier)

		self.offset = int(field_cursor.get_field_offsetof() / 8)

		self.array_count = field_cursor.type.get_array_size()

		for field_token in field_cursor.get_children():
			if field_token.kind == clang.CursorKind.ANNOTATE_ATTR:
				self.attributes.append(field_token.displayname)


	def __json__(self):
		rep = {}

		rep["name"] = self.name
		rep["type"] = self.type
		rep["access"] = self.access
		rep["is_pointer"] = self.is_pointer
		rep["is_reference"] = self.is_reference
		rep["is_const"] = self.is_const
		rep["byte_offset"] = self.offset
		rep["array_size"] = self.array_count
		rep["attributes"] = self.attributes
		
		return rep


class CppModifiedType:
	def __init__(self, type: clang.Type):
		self.type: str = CppType.read_type(type)
		self.is_pointer: bool = type.kind == clang.TypeKind.POINTER
		self.is_reference: bool = type.kind == clang.TypeKind.LVALUEREFERENCE
		self.is_const: bool = type.is_const_qualified()
	

	def __json__(self):
		rep = {}

		rep["type"] = self.type
		rep["is_pointer"] = self.is_pointer
		rep["is_reference"] = self.is_reference
		rep["is_const"] = self.is_const

		return rep


class CppMethod:
	def __init__(self, method_cursor: clang.Cursor):
		self.name = method_cursor.spelling

		self.return_type = CppModifiedType(method_cursor.type.get_result().get_canonical())

		self.arguments = []

		self.is_abstract = method_cursor.is_virtual_method()
		self.is_pure_virtual = method_cursor.is_pure_virtual_method()

		self.access = str_access_specifier(method_cursor.access_specifier)

		self.is_const = method_cursor.is_const_method()

		for arg in method_cursor.type.argument_types():
			self.arguments.append(CppModifiedType(arg.get_canonical()))

	def __json__(self):
		return { "name": self.name, "return_type": self.return_type, "arguments": self.arguments, "is_virtual": self.is_abstract, "is_pure_virtual": self.is_pure_virtual, "access": self.access, "is_const": self.is_const }


class CppType:
	all_types: dict[str, Self] = {}

	@classmethod
	def read_type(cls, type: clang.Type) -> str:
		name: str = type.spelling
		
		if name == "":
			return ""

		discard = False

		if name.endswith("const"):
			name = name.removesuffix("const").strip()

			discard = True
		
		if name.startswith("const"):
			name = name.removeprefix("const").strip()

			discard = True

		if name.endswith("*"):
			name = name.strip("* ")

			discard = True

		if name.endswith("&"):
			name = name.strip("& ")

			discard = True

		if name.endswith("]"):
			name = name.strip("[]0123456789")

			discard = True

		if type.kind.value < 100:
			discard = True

		if type.get_declaration().kind == clang.CursorKind.NO_DECL_FOUND:
			discard = True

		if "(unnamed" in type.spelling:
			discard = True

		if discard:
			return name
		
		if name not in CppType.all_types:
			CppType.all_types[name] = None
	
			CppType.all_types[name] = CppType(type)

		return name
		
	@classmethod
	def read_all_types(cls, code_file: clang.TranslationUnit):
		cursor: clang.Cursor = code_file.cursor

		definitions: list[clang.Cursor] = [
			token for token in cursor.get_children() if (
				is_type_decl(token) and token.is_definition()
			)
		]

		for class_def in definitions:
			type: clang.Type = class_def.type
			
			if HEADER_FILES_DIRECTORY not in type.get_declaration().location.file.name:
				continue

			cls.read_type(type)
			
	
	def __init__(self, clang_type: clang.Type):
		decl_cursor: clang.Cursor = clang_type.get_declaration()

		self.cursor: clang.Cursor = decl_cursor
		self.name: str = clang_type.spelling
		self.enclosing_class = CppType.read_type(decl_cursor.lexical_parent.type) if is_type_decl(decl_cursor) else None
		self.fields: list[CppField] = []
		self.methods: list[CppMethod] = []
		self.base_classes: list[str] = []
		self.constructors: list[CppMethod] = []
		self.destructor: CppMethod = None
		self.template_args: list = []
		self.is_enum = decl_cursor.kind == clang.CursorKind.ENUM_DECL
		self.enum_width = clang_type.get_size() if self.is_enum else 0

		for class_part in decl_cursor.get_children():
			if is_type_decl(class_part):
				CppType.read_type(class_part.type)
			elif class_part.kind == clang.CursorKind.FIELD_DECL:
				self.fields.append(CppField(class_part))
			elif class_part.kind == clang.CursorKind.CXX_METHOD:
				self.methods.append(CppMethod(class_part))
			elif class_part.kind == clang.CursorKind.CXX_BASE_SPECIFIER:
				self.base_classes.append(CppType.read_type(class_part.type))
			elif class_part.kind == clang.CursorKind.CONSTRUCTOR:
				self.constructors.append(CppMethod(class_part))
			elif class_part.kind == clang.CursorKind.DESTRUCTOR:
				self.destructor = CppMethod(class_part)
		
		for template_arg_index in range(self.cursor.get_num_template_arguments()):
			if self.cursor.get_template_argument_kind(template_arg_index) == clang.TemplateArgumentKind.INTEGRAL:
				self.template_args.append(self.cursor.get_template_argument_value(template_arg_index))
			else:
				self.template_args.append(CppModifiedType(self.cursor.get_template_argument_type(template_arg_index)))


	def get_simple_name(self) -> str:
		name = self.cursor.spelling

		if self.cursor.lexical_parent:
			parent = self.cursor.lexical_parent
			
			while parent:
				if parent.kind == clang.CursorKind.NAMESPACE or is_type_decl(parent):
					name = parent.spelling + "::" + name
				else:
					break	
			
				parent = parent.lexical_parent
		
		return name


	def __json__(self):
		rep = {}
		rep["fields"] = self.fields
		rep["methods"] = self.methods
		rep["base_classes"] = self.base_classes
		rep["constructors"] = self.constructors
		rep["destructor"] = self.destructor
		rep["source"] = self.cursor.location.file.name
		rep["name"] = self.name
		rep["simple_name"] = self.get_simple_name()
		rep["template_args"] = self.template_args
		rep["projects_own"] = HEADER_FILES_DIRECTORY in self.cursor.location.file.name
		rep["access"] = str_access_specifier(self.cursor.access_specifier)
		rep["enclosing_class"] = self.enclosing_class
		rep["enum_width"] = self.enum_width

		return rep


def is_type_decl(cursor: clang.Cursor) -> bool:
	return cursor.kind == clang.CursorKind.CLASS_DECL or cursor.kind == clang.CursorKind.STRUCT_DECL


def str_full_type_name(type: clang.Type) -> str:
	name = type.spelling

	parent: clang.Cursor = type.get_declaration().lexical_parent

	if not parent:
		return name

	if is_type_decl(parent):
		name = str_full_type_name(parent.type) + "::" + name
	elif parent.kind == clang.CursorKind.NAMESPACE:
		name = parent.spelling + "::" + name

	return name


def str_access_specifier(access_specifier: clang.AccessSpecifier) -> str:
	if access_specifier == clang.AccessSpecifier.PUBLIC or access_specifier == clang.AccessSpecifier.INVALID:
		return "public"
	elif access_specifier == clang.AccessSpecifier.PROTECTED:
		return "protected"
	else:
		return "private"


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
		for message in diagnostic_messages:
			print(message)
		
		raise RuntimeError()

	return result


def main():
	print("Generating type database...")

	compile_args: list[str] = []

	compile_commands: list = []

	with open(COMMAND_FILE, "r") as compile_commands_file:
		compile_commands = json.load(compile_commands_file)

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
					rsp_path = path.abspath(COMMAND_FILE + "/../src/" + arg[1:])

					if path.exists(rsp_path):
						with open(rsp_path, "r") as rsp_file:
							rsp_args = rsp_file.readline()
							rsp_args = rsp_args.removesuffix("\n")

							rsp_args_array = rsp_args.split(" ")

							rsp_args_array = [ file_path.removeprefix("\"").removesuffix("\"") for file_path in rsp_args_array ]

							compile_args += rsp_args_array

			break

	files = [path.abspath(HEADER_FILES_DIRECTORY + "/" + file) for file in os.listdir(HEADER_FILES_DIRECTORY)]

	CppType.read_all_types(construct_file(files, compile_args))
	
	with open(SOURCE_FILES_DIRECTORY + "/codegen/type_database.json", "w") as json_file:
		json.dump(CppType.all_types, json_file, indent=2, default=lambda o: o.__json__() if hasattr(o, '__json__') else None)

	print("\tDone!")

	
if __name__ == "__main__":
	try:
		main()
	except Exception as e:
		print(traceback.format_exc())

		exit(1)

	
