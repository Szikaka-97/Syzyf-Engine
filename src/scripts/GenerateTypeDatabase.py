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

class CppType:
	def __init__(self, clang_type: clang.Type):
		self.clang_type: clang.Type = clang_type
		self.is_const: bool = clang_type.is_const_qualified()
		self.name: str = clang_type.get_declaration().spelling if (clang_type.kind == clang.TypeKind.ELABORATED or clang_type.kind == clang.TypeKind.RECORD) else clang_type.spelling

		self.full_name: str = self.name

		full_name_cursor = clang_type.get_declaration().semantic_parent

		while full_name_cursor and full_name_cursor.kind.is_declaration() and full_name_cursor.kind != clang.CursorKind.LINKAGE_SPEC:
			self.full_name = full_name_cursor.spelling + "::" + self.full_name

			full_name_cursor = full_name_cursor.semantic_parent

		self.type_def: CppClass = None
		self.pointed_type: Self = None

		self.is_anonymous: str = clang_type.get_declaration().is_anonymous()
		self.is_union = clang_type.get_declaration().kind == clang.CursorKind.UNION_DECL
		self.is_pointer = clang_type.kind == clang.TypeKind.POINTER
		self.is_reference = clang_type.kind == clang.TypeKind.LVALUEREFERENCE

		self.template_args = []

		if clang_type.get_declaration().get_num_template_arguments() > 0:
			self.full_name += "<"

			for i in range(clang_type.get_declaration().get_num_template_arguments()):
				if clang_type.get_declaration().get_template_argument_kind(i) == clang.TemplateArgumentKind.INTEGRAL:
					# dirty glm::vec fix
					if self.full_name.startswith("glm::") and i == clang_type.get_declaration().get_num_template_arguments() - 1:
						self.template_args.append("glm::defaultp")
					else:
						self.template_args.append(clang_type.get_declaration().get_template_argument_value(i))

					self.full_name += str(self.template_args[-1])
				else:
					self.template_args.append(CppType(clang_type.get_declaration().get_template_argument_type(i).get_canonical()))

					self.full_name += self.template_args[-1].full_name

				if i < clang_type.get_declaration().get_num_template_arguments() - 1:
					self.full_name += ", "
			
			self.full_name += ">"

		if self.is_anonymous and not self.is_union:
			self.type_def = CppClass(clang_type.get_declaration())
		
		self.is_enum = clang_type.get_declaration().kind == clang.CursorKind.ENUM_DECL
		self.enum_width = clang_type.get_size() if self.is_enum else 0

		if self.is_pointer or self.is_reference:
			self.pointed_type = CppType(clang_type.get_pointee())
		else:
			self.pointed_type = None
	

	def __json__(self):
		rep = { "is_const": self.is_const, "is_union": self.is_union, "is_pointer": self.is_pointer, "is_reference": self.is_reference, "is_enum": self.is_enum, "enum_width": self.enum_width }

		if len(self.template_args) > 0:
			rep["template_args"] = self.template_args

		if not self.is_anonymous:
			rep["name"] = self.name
			rep["full_name"] = self.full_name

		if self.is_pointer or self.is_reference:
			rep["pointed_type"] = self.pointed_type
		
		if self.is_anonymous and not self.is_union:
			rep["class_def"] = self.type_def

		return rep


class CppField:
	def __init__(self, field_cursor: clang.Cursor):
		self.name = field_cursor.spelling

		self.type = CppType(field_cursor.type.get_canonical())

		self.attributes = []

		self.access = str_access_specifier(field_cursor.access_specifier)

		self.offset = int(field_cursor.get_field_offsetof() / 8)

		for field_token in field_cursor.get_children():
			if field_token.kind == clang.CursorKind.ANNOTATE_ATTR:
				self.attributes.append(field_token.displayname)

	
	def __json__(self):
		return { "name": self.name, "type": self.type, "attributes": self.attributes, "access": self.access, "byte_offset": self.offset }


class CppMethod:
	def __init__(self, method_cursor: clang.Cursor):
		self.name = method_cursor.spelling

		self.return_type = CppType(method_cursor.type.get_result().get_canonical())

		self.argument_types = []

		self.is_abstract = method_cursor.is_virtual_method()
		self.is_pure_virtual = method_cursor.is_pure_virtual_method()

		self.access = str_access_specifier(method_cursor.access_specifier)

		self.is_const = method_cursor.is_const_method()

		for arg in method_cursor.type.argument_types():
			self.argument_types.append(CppType(arg.get_canonical()))

	def __json__(self):
		return { "name": self.name, "return_type": self.return_type, "argument_types": self.argument_types, "is_virtual": self.is_abstract, "is_pure_virtual": self.is_pure_virtual, "access": self.access, "is_const": self.is_const }


class CppClass:
	all_classes: dict[str, Self] = {}

	@classmethod
	def read_all_classes(cls, code_file: clang.TranslationUnit):
		cursor: clang.Cursor = code_file.cursor

		definitions = [
			token for token in cursor.get_children() if (
				(token.kind == clang.CursorKind.CLASS_DECL or token.kind == clang.CursorKind.STRUCT_DECL) and token.is_definition()
			)
		]

		for class_def in definitions:
			gened_class = CppClass(class_def)

			CppClass.all_classes[gened_class.get_full_name()] = CppClass(class_def)
		
		CppClass.all_classes = {key: value for key, value in CppClass.all_classes.items() if HEADER_FILES_DIRECTORY in value.cursor.location.file.name}

		for cpp_cls in CppClass.all_classes:
			CppClass.all_classes[cpp_cls].populate()
			

	def __init__(self, class_cursor: clang.Cursor, enclosing_class: Self = None):
		if class_cursor.kind != clang.CursorKind.CLASS_DECL and class_cursor.kind != clang.CursorKind.STRUCT_DECL:
			raise RuntimeError("Provided cursor is not a class definition cursor, but instead " + str(class_cursor.kind))

		self.cursor = class_cursor
		self.name: str = class_cursor.spelling
		self.enclosing_class = enclosing_class
		self.fields: list[CppField] = []
		self.methods: list[CppMethod] = []
		self.parent_classes: list[Self] = []
		self.constructors: list[CppMethod] = []
		self.destructor: CppMethod = None

		self.is_abstract = False

		self.template_args = []

		if class_cursor.semantic_parent and class_cursor.semantic_parent.kind == clang.CursorKind.NAMESPACE:
			namespace_cursor = class_cursor.semantic_parent

			while namespace_cursor and namespace_cursor.kind == clang.CursorKind.NAMESPACE:
				self.name = namespace_cursor.spelling + "::" + self.name

				namespace_cursor = namespace_cursor.semantic_parent

		for i in range(class_cursor.get_num_template_arguments()):
			if class_cursor.get_template_argument_kind(i) == clang.TemplateArgumentKind.INTEGRAL:
				self.template_args.append(class_cursor.get_template_argument_value(i))
			else:
				self.template_args.append(CppType(class_cursor.get_template_argument_type(i).get_canonical()))

		for class_part in class_cursor.get_children():
			if (class_part.kind == clang.CursorKind.CLASS_DECL or class_part.kind == clang.CursorKind.STRUCT_DECL) and class_part.is_definition():
				nested_cls = CppClass(class_part, self)

				CppClass.all_classes[nested_cls.get_full_name()] = nested_cls
	

	def get_full_name(self) -> str:
		full_name = self.name if not self.enclosing_class else self.enclosing_class.get_full_name() + "::" + self.name

		if len(self.template_args) > 0:
			full_name += "<"

			for i in range(len(self.template_args)):
				if isinstance(self.template_args[i], CppType):
					full_name += self.template_args[i].full_name
				else:
					full_name += self.template_args[i]

				if i < len(self.template_args) - 1:
					full_name += ", "
			
			full_name += ">"

		return full_name


	def populate(self) -> None:
		for class_part in self.cursor.get_children():
			if class_part.kind == clang.CursorKind.CXX_BASE_SPECIFIER:
				if class_part.spelling in CppClass.all_classes:
					self.parent_classes.append(CppClass.all_classes[class_part.spelling])
			elif class_part.kind == clang.CursorKind.FIELD_DECL:
				self.fields.append(CppField(class_part))
			elif class_part.kind == clang.CursorKind.CXX_METHOD:
				self.methods.append(CppMethod(class_part))
			elif class_part.kind == clang.CursorKind.CONSTRUCTOR:
				self.constructors.append(CppMethod(class_part))
			elif class_part.kind == clang.CursorKind.DESTRUCTOR:
				self.destructor = CppMethod(class_part)
	

	def __json__(self):
		rep = {}
		
		rep["name"] = self.name
		rep["base_classes"] = [base.get_full_name() for base in self.parent_classes]
		rep["enclosing_class"] = self.enclosing_class.get_full_name() if self.enclosing_class else ""
		rep["fields"] = self.fields
		rep["methods"] = self.methods
		rep["constructors"] = self.constructors
		rep["destructor"] = self.destructor
		rep["source"] = self.cursor.location.file.name

		rep["access"] = str_access_specifier(self.cursor.access_specifier)

		return rep


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
		compile_commands: list = json.load(compile_commands_file)
	
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

	CppClass.read_all_classes(construct_file(files, compile_args))

	with open(SOURCE_FILES_DIRECTORY + "/codegen/type_database.json", "w") as json_file:
		json.dump(CppClass.all_classes, json_file, indent=2, default=lambda o: o.__json__() if hasattr(o, '__json__') else None)
	

	print("\tDone!")
	
if __name__ == "__main__":
	try:
		main()
	except Exception as e:
		print(traceback.format_exc())

		exit(1)

	
