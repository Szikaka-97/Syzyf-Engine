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

		all_classes[class_name] = cls


def main():
	global all_classes, data

	print("Generating TypeInfo structures...")
	
	with open(SOURCE_TYPE_DATABASE) as json_file:
		data = json.load(json_file)

		for class_name, cls_data in data.items():
			load_class(class_name, cls_data)

	with CodeWriter(DEST_SOURCE_FILE_PATH) as dest_impl:
		dest_impl.line("#include <TypeInfo.h>")
		dest_impl.line()
		dest_impl.line("#include <unordered_map>")
		dest_impl.line("#include <typeindex>")
		dest_impl.line()

		include_files = []

		for cls_name, cls in all_classes.items():
			if cls.source not in include_files:
				include_files.append(cls.source)
		
		for include_file in include_files:
			dest_impl.line(f"#include \"{include_file}\"")
		
		dest_impl.line()
		dest_impl.line("TypeInfo allTypeInfos[] {")
		dest_impl.more_indent()

		for cls_name, cls in all_classes.items():
			if cls.access == "public":
				dest_impl.line(f"{{ .name = \"{cls_name}\", .size = sizeof({cls_name}) }},")

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("std::unordered_map<std::string, int> typeInfoLookupByName {")
		dest_impl.more_indent()

		index = 0
		for cls_name, cls in all_classes.items():
			if cls.access == "public":
				dest_impl.line(f"{{ \"{cls_name}\", {index} }},")
				index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("std::unordered_map<std::type_index, int> typeInfoLookupByTypeId {")
		dest_impl.more_indent()

		index = 0
		for cls_name, cls in all_classes.items():
			if cls.access == "public":
				dest_impl.line(f"{{ typeid({cls_name}), {index} }},")
				index += 1

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("const TypeInfo& GetTypeInfo(const std::string& typeName) {")
		dest_impl.more_indent()

		dest_impl.line("return allTypeInfos[typeInfoLookupByName[typeName]];")

		dest_impl.less_indent()
		dest_impl.line("};")

		dest_impl.line("const TypeInfo& GetTypeInfo(const std::type_info& typeInfo) {")
		dest_impl.more_indent()

		dest_impl.line("return allTypeInfos[typeInfoLookupByTypeId[typeInfo]];")

		dest_impl.less_indent()
		dest_impl.line("};")
	
	print("\tDone!")


if __name__ == "__main__":
	main()