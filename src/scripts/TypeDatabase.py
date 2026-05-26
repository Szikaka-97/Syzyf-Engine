from typing import Any, Self

class CppModifiedType:
	def __init__(self, data: dict[str, Any]):
		self.type: str = data["type"]
		self.is_pointer: bool = data["is_pointer"]
		self.is_reference: bool = data["is_reference"]
		self.is_const: bool = data["is_const"]


class CppField:
	def __init__(self, data: dict[str, Any]):
		self.name: str = data["name"]
		self.type: str = data["type"]
		self.attributes: list[str] = data["attributes"]
		self.access: str = data["access"]
		self.offset: int = data["byte_offset"]
		self.array_count: int = data["array_size"]
		self.is_pointer: bool = data["is_pointer"]
		self.is_reference: bool = data["is_reference"]
		self.is_const: bool = data["is_const"]


class CppMethod:
	def __init__(self, data: dict[str, Any]):
		self.name: str = data["name"]
		self.return_type: CppModifiedType = CppModifiedType(data["return_type"])
		self.arguments: list[CppModifiedType] = [CppModifiedType(type_data) for type_data in data["arguments"]]
		self.is_virtual: bool = data["is_virtual"]
		self.is_pure_virtual: bool = data["is_pure_virtual"]
		self.is_const: bool = data["is_const"]
		self.access: str = data["access"]


class CppType:
	all_types: dict[str, Self] = {}

	@classmethod
	def load_types(cls, data: dict[str, dict]) -> Self:
		for type_name, type_data in data.items():
			cls.all_types[type_name] = CppType(type_data)

	@classmethod
	def get_type(cls, type_name: str) -> Self:
		return cls.all_types[type_name]


	def __init__(self, data: dict[str, Any]):
		self.fields: list[CppField] = [CppField(field_data) for field_data in data["fields"]]
		self.methods: list[CppMethod] = [CppMethod(method_data) for method_data in data["methods"]]
		self.base_classes: list[str] = data["base_classes"]
		self.constructors: list[CppMethod] = [CppMethod(constructor_data) for constructor_data in data["constructors"]]
		self.destructor: CppMethod = CppMethod(data["destructor"]) if data["destructor"] else None
		self.source: str = data["source"]
		self.name: str = data["name"]
		self.simple_name: str = data["simple_name"]
		self.template_args: list = [arg if isinstance(arg, int) else CppModifiedType(arg) for arg in data["template_args"]]
		self.projects_own: bool = data["projects_own"]
		self.access: str = data["access"]
		self.enclosing_class: str = data["enclosing_class"]
		self.enum_width: int = data["enum_width"]


	def default_constructible(self) -> bool:
		return any(constructor for constructor in self.constructors if constructor.access == "public" and len(constructor.arguments) == 0)


	def is_enum(self) -> bool:
		return self.enum_width > 0


	def is_polymorphic(self) -> bool:
		self_abstract = any([met.is_virtual for met in self.methods]) or (self.destructor and self.destructor.is_virtual)

		return self_abstract or any([CppType.get_type(base).is_abstract() for base in self.base_classes])
	

	def _get_pure_virtual_methods(self) -> list[CppMethod]:
		pure_methods = [method for method in self.methods if method.is_pure_virtual]

		for base in self.base_classes:
			pure_methods += CppType.get_type(base)._get_pure_virtual_methods()

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
			queue = [CppType.get_type(base) for base in queue[-1].base_classes] + queue

			hierarchy.append(queue.pop())
		
		return hierarchy


	def get_all_fields(self) -> list[CppField]:
		return [field for base_class in self.get_class_hierarchy() for field in base_class.fields]
	

	def get_all_methods(self) -> list[CppMethod]:
		return [method for base_class in self.get_class_hierarchy() for method in base_class.methods]
