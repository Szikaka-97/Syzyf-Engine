from typing import Any, Self

class CppType:
	def __init__(self, data: dict[str, Any]):
		self.is_const: bool = data["is_const"]
		self.is_union: bool = data["is_union"]
		self.is_pointer: bool = data["is_pointer"]
		self.is_reference: bool = data["is_reference"]

		self.template_args: list = data["template_args"] if "template_args" in data else []
		self.name: str = data["name"] if "name" in data else ""
		self.full_name: str = data["full_name"] if "full_name" in data else ""

		self.pointed_type: CppType = CppType(data["pointed_type"]) if "pointed_type" in data else None
		self.class_def: CppClass = CppClass(data["class_def"]) if "class_def" in data else None


class CppField:
	def __init__(self, data: dict[str, Any]):
		self.name: str = data["name"]
		self.type: CppType = CppType(data["type"])
		self.attributes: list[str] = data["attributes"]
		self.access: str = data["access"]
		self.offset: int = data["byte_offset"]


class CppMethod:
	def __init__(self, data: dict[str, Any]):
		self.name: str = data["name"]
		self.return_type: CppType = CppType(data["return_type"])
		self.argument_types: list[CppType] = [CppType(type_data) for type_data in data["argument_types"]]
		self.is_virtual: bool = data["is_virtual"]
		self.access: str = data["access"]


class CppClass:
	def __init__(self, data: dict[str, Any]):
		self.name: str = data["name"]
		self.base_classes: list[CppClass] = []
		self.enclosing_class: CppClass = None
		self.fields: list[CppField] = [CppField(field_data) for field_data in data["fields"]]
		self.methods: list[CppMethod] = [CppMethod(method_data) for method_data in data["methods"]]
		self.source: str = data["source"]
		self.access: str = data["access"]
	

	def get_full_name(self) -> str:
		if self.enclosing_class == None:
			return self.name

		return self.enclosing_class.get_full_name() + "::" + self.name


	def is_abstract(self) -> bool:
		return any([met.is_virtual for met in self.methods])
	

	def get_class_hierarchy(self) -> list[Self]:
		hierarchy = []
		queue = [self]

		while len(queue):
			queue += queue[-1].base_classes

			hierarchy.append(queue.pop())
		
		return hierarchy


	def get_all_fields(self) -> list[CppField]:
		return [field for base_class in self.get_class_hierarchy() for field in base_class.fields]
	

	def get_all_methods(self) -> list[CppMethod]:
		return [method for base_class in self.get_class_hierarchy() for method in base_class.methods]