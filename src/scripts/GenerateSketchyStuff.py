import json
import sys
import clang.cindex as clang # type: ignore
from typing import Self
from enum import Enum
import os
from os import path
import traceback
from TypeDatabase import *

SOURCE_TYPE_DATABASE = sys.argv[1]
DEST_HEADER_FILE_PATH = os.path.dirname(sys.argv[1]) + "/MessagingDecls.h"
DEST_SOURCE_FILE_PATH = os.path.dirname(sys.argv[1]) + "/MessagingDecls.cpp"

gameobject_types = []
scenecomponent_types = []


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
	global gameobject_types, scenecomponent_types

	print("Generating Stuff...")

	with open(SOURCE_TYPE_DATABASE) as json_file:
		data = json.load(json_file)

		CppType.load_types(data)

	gameobject_types = [type for type in CppType.all_types.values() if any([game_object_base for game_object_base in type.get_class_hierarchy() if game_object_base.name == "GameObject"])]

	scenecomponent_types = [type for type in CppType.all_types.values() if any([scene_component_base for scene_component_base in type.get_class_hierarchy() if scene_component_base.name == "SceneComponent" or "GameObjectSystem" in scene_component_base.name])]

	with CodeWriter(DEST_SOURCE_FILE_PATH) as dest:
		dest.line("#include <string>")
		dest.line("#include <map>")
		dest.line("#include <functional>")
		dest.line("#include <TypeInfo.h>")

		dest.line()

		include_files = []

		for tp in gameobject_types:
			if tp.source not in include_files:
				include_files.append(tp.source)

		for tp in scenecomponent_types:
			if tp.source not in include_files:
				include_files.append(tp.source)
		
		for include_file in include_files:
			dest.line(f"#include \"{include_file}\"")
		

		dest.line()

		dest.line("class SceneNode;")
		dest.line("class Scene;")

		dest.line()

		dest.line()

		dest.line("class MessagingHelpers {")
		dest.line("public:")
		dest.more_indent()

		dest.line("static std::map<std::string, std::function<GameObject* (SceneNode*)>> gameObjectAdditionFunctions;")
		dest.line("static std::map<std::string, std::function<SceneComponent* (Scene*)>> sceneComponentAdditionFunctions;")
		dest.line("static std::map<std::string, std::function<void (SceneNode* node, GameObject*)>> gameObjectAttachmentFunctions;")

		dest.less_indent()
		dest.line("};")

		dest.line()

		dest.line("std::map<std::string, std::function<SceneComponent* (Scene*)>> MessagingHelpers::sceneComponentAdditionFunctions {")
		dest.more_indent()

		for tp in scenecomponent_types:
			if not tp.is_abstract() and "GameObjectSystem<" not in tp.name:
				dest.line(f"{{ \"{tp.name}\", [](Scene* scene) -> SceneComponent* {{ return scene->AddComponent<{tp.name}>(); }} }},")

		dest.less_indent()
		dest.line("};")

		dest.line("std::map<std::string, std::function<void (SceneNode*, GameObject*)>> MessagingHelpers::gameObjectAttachmentFunctions {")
		dest.more_indent()

		for tp in gameobject_types:
			lineSpelling = f"{{ \"{tp.name}\", [](SceneNode* node, GameObject* obj) -> void {{ return node->GetScene()->AddGameObjectInternal<{tp.name}>(node, dynamic_cast<{tp.name} *>(obj)); }} }},"

			if any([constructor for constructor in tp.constructors if len(constructor.arguments) == 0]):
				dest.line(lineSpelling)
			else:
				dest.line("//" + lineSpelling)

		dest.less_indent()
		dest.line("};")

		dest.line("std::map<std::string, std::function<GameObject* (SceneNode*)>> MessagingHelpers::gameObjectAdditionFunctions {")
		dest.more_indent()

		for tp in gameobject_types:
			lineSpelling = f"{{ \"{tp.name}\", [](SceneNode* node) -> GameObject* {{ return node->AddObject<{tp.name}>(); }} }},"

			if any([constructor for constructor in tp.constructors if len(constructor.arguments) == 0]):
				dest.line(lineSpelling)
			else:
				dest.line("//" + lineSpelling)

		dest.less_indent()
		dest.line("};")

		dest.line("")

		dest.line("GameObject* MessagingHelpers_AttachObjectToNode(SceneNode* node, GameObject* obj) {")
		dest.more_indent()
		
		dest.line("auto objectName = TypeInfo::GetTypeInfo(typeid(*obj)).name;")
		dest.line("auto func = MessagingHelpers::gameObjectAdditionFunctions.find(objectName);")

		dest.line("if (func != MessagingHelpers::gameObjectAdditionFunctions.end()) {")
		dest.more_indent()
		
		dest.line("return func->second(node);")

		dest.less_indent()
		dest.line("}")
		dest.line("return nullptr;")

		dest.less_indent()
		dest.line("}")
		dest.line()

		dest.line("GameObject* MessagingHelpers_AddObjectToNode(SceneNode* node, const std::string& objectName) {")
		dest.more_indent()
		
		dest.line("auto func = MessagingHelpers::gameObjectAdditionFunctions.find(objectName);")

		dest.line("if (func != MessagingHelpers::gameObjectAdditionFunctions.end()) {")
		dest.more_indent()
		
		dest.line("return func->second(node);")

		dest.less_indent()
		dest.line("}")
		dest.line("return nullptr;")

		dest.less_indent()
		dest.line("}")
		dest.line()

		dest.line("SceneComponent* MessagingHelpers_AddComponentToScene(Scene* scene, const std::string& objectName) {")
		dest.more_indent()
		
		dest.line("auto func = MessagingHelpers::sceneComponentAdditionFunctions.find(objectName);")

		dest.line("if (func != MessagingHelpers::sceneComponentAdditionFunctions.end()) {")
		dest.more_indent()
		
		dest.line("return func->second(scene);")

		dest.less_indent()
		dest.line("}")
		dest.line("return nullptr;")

		dest.less_indent()
		dest.line("}")
		dest.line()

		dest.line("std::vector<std::string> MessagingHelpers_GetAvailableComponents() {")
		dest.more_indent()
		
		dest.line("std::vector<std::string> result;")

		dest.line("for (const auto& pair : MessagingHelpers::sceneComponentAdditionFunctions) {")
		dest.more_indent()
		
		dest.line("result.push_back(pair.first);")

		dest.less_indent()
		dest.line("}")
		dest.line()
		dest.line("return result;")

		dest.less_indent()
		dest.line("}")
		dest.line()

		dest.line("std::vector<std::string> MessagingHelpers_GetAvailableGameObjects() {")
		dest.more_indent()
		
		dest.line("std::vector<std::string> result;")

		dest.line("for (const auto& pair : MessagingHelpers::gameObjectAdditionFunctions) {")
		dest.more_indent()
		
		dest.line("result.push_back(pair.first);")

		dest.less_indent()
		dest.line("}")
		dest.line()
		dest.line("return result;")

		dest.less_indent()
		dest.line("}")
		dest.line()
		

	print("\tDone!")


if __name__ == "__main__":
	main()