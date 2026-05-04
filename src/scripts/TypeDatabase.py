from typing import Any, Self

class CppType:
	def __init__(self, data: dict[str, Any]):
		self.is_const: bool = data["is_const"]
		self.is_union: bool = data["is_union"]
		self.is_pointer: bool = data["is_pointer"]
		self.is_reference: bool = data["is_reference"]
		self.is_enum: bool = data["is_enum"]
		self.enum_width:int = data["enum_width"]

		self.template_args: list = [
			(CppType(template_param_data) if isinstance(template_param_data, dict) else template_param_data) for template_param_data in data["template_args"]
		] if "template_args" in data else []
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
		self.is_pure_virtual = data["is_pure_virtual"]
		self.access: str = data["access"]


class CppClass:
	def __init__(self, data: dict[str, Any]):
		self.name: str = data["name"]
		self.base_classes: list[CppClass] = []
		self.enclosing_class: CppClass = None
		self.fields: list[CppField] = [CppField(field_data) for field_data in data["fields"]]
		self.methods: list[CppMethod] = [CppMethod(method_data) for method_data in data["methods"]]
		self.constructors: list[CppMethod] = [CppMethod(constructor_data) for constructor_data in data["constructors"]]
		self.destructor: CppMethod = CppMethod(data["destructor"]) if data["destructor"] else None
		self.source: str = data["source"]
		self.access: str = data["access"]
	

	def get_full_name(self) -> str:
		if self.enclosing_class == None:
			return self.name

		return self.enclosing_class.get_full_name() + "::" + self.name


	def is_polymorphic(self) -> bool:
		self_abstract = any([met.is_virtual for met in self.methods]) or (self.destructor and self.destructor.is_virtual)

		return self_abstract or any([base.is_abstract() for base in self.base_classes])
	

	def _get_pure_virtual_methods(self) -> list[CppMethod]:
		pure_methods = [method for method in self.methods if method.is_pure_virtual]

		for base in self.base_classes:
			pure_methods += base._get_pure_virtual_methods()

		unimplemented_methods = []

		for pure in pure_methods:
			if not any([method for method in self.methods if method.name == pure.name and not method.is_pure_virtual]):
				unimplemented_methods.append(pure)

		return unimplemented_methods


	def is_abstract(self) -> bool:
		return len(self._get_pure_virtual_methods()) > 0


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