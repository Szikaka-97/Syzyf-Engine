import sys
import os
import os.path
from typing import Self
from enum import Enum
import json
import re as regex
from TypeDatabase import *

data: dict[str, dict] = None

SOURCE_TYPE_DATABASE = sys.argv[1]
DEST_SOURCE_FILE_PATH = os.path.dirname(sys.argv[1]) + "/TypeInfo.cpp"

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


def main():
	global all_classes, data

	print("Generating TypeInfo structures...")
	
	with open(SOURCE_TYPE_DATABASE) as json_file:
		data = json.load(json_file)

		CppType.load_types(data)

	with CodeWriter(DEST_SOURCE_FILE_PATH) as dest_impl:
		dest_impl.line("#include <TypeInfo.h>")
		dest_impl.line()
		dest_impl.line("#include <unordered_map>")
		dest_impl.line("#include <typeindex>")
		dest_impl.line()

		include_files = []

		CppType.all_types = {type_name: tp for type_name, tp in CppType.all_types.items() if tp.projects_own and tp.access == "public" and not tp.enclosing_class}

		for type_name, tp in CppType.all_types.items():
			if tp.source not in include_files:
				include_files.append(tp.source)
		
		for include_file in include_files:
			dest_impl.line(f"#include \"{include_file}\"")
		
		dest_impl.line()
		dest_impl.line("TypeInfo allTypeInfos[] {")
		dest_impl.more_indent()

		for type_name, tp in CppType.all_types.items():
			dest_impl.line(f"{{ .name = \"{type_name}\", .size = sizeof({type_name}) }},")

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("std::unordered_map<std::string, int> typeInfoLookupByName {")
		dest_impl.more_indent()

		index = 0
		for type_name in CppType.all_types:
			dest_impl.line(f"{{ \"{type_name}\", {index} }},")
			index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("std::unordered_map<std::type_index, int> typeInfoLookupByTypeId {")
		dest_impl.more_indent()

		index = 0
		for type_name in CppType.all_types:
			dest_impl.line(f"{{ typeid({type_name}), {index} }},")
			dest_impl.line(f"{{ typeid({type_name} *), {index} }},")
			dest_impl.line(f"{{ typeid(const {type_name} *), {index} }},")
			index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("const TypeInfo& TypeInfo::GetTypeInfo(const std::string& typeName) {")
		dest_impl.more_indent()

		dest_impl.line("return allTypeInfos[typeInfoLookupByName[typeName]];")

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("const TypeInfo& TypeInfo::GetTypeInfo(const std::type_info& typeInfo) {")
		dest_impl.more_indent()

		dest_impl.line("return allTypeInfos[typeInfoLookupByTypeId[std::type_index{typeInfo}]];")

		dest_impl.less_indent()
		dest_impl.line("};")
	
	print("\tDone!")


if __name__ == "__main__":
	main()