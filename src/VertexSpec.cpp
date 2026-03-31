#include <VertexSpec.h>

#include <map>

VertexInputType VertexSpec::TypeFromSemantic(const std::string& s) {
	const static std::map<std::string, VertexInputType> nameToTypeMap({
			{ "POSITION", VertexInputType::Position },
			{ "NORMAL", VertexInputType::Normal },
			{ "BINORMAL", VertexInputType::Binormal },
			{ "TANGENT", VertexInputType::Tangent },
			{ "UV1", VertexInputType::UV1 },
			{ "UV2", VertexInputType::UV2 },
			{ "COLOR", VertexInputType::Color },
      { "JOINTS", VertexInputType::Joints },
      { "WEIGHTS", VertexInputType::Weights },
		});

	if (nameToTypeMap.contains(s)) {
		return nameToTypeMap.at(s);
	}

	return VertexInputType::Invalid;
}

VertexInputType VertexSpec::TypeFromName(const std::string& s) {
	const static std::map<std::string, VertexInputType> nameToTypeMap({
			{ "Position", VertexInputType::Position },
			{ "Normal", VertexInputType::Normal },
			{ "Binormal", VertexInputType::Binormal },
			{ "Tangent", VertexInputType::Tangent },
			{ "UV1", VertexInputType::UV1 },
			{ "UV2", VertexInputType::UV2 },
			{ "Color", VertexInputType::Color },
      { "Joints", VertexInputType::Joints },
      { "Weights", VertexInputType::Weights },
		});

	if (nameToTypeMap.contains(s)) {
		return nameToTypeMap.at(s);
	}

	return VertexInputType::Invalid;
}

const std::string& VertexSpec::TypeToName(VertexInputType t) {
	static std::map<VertexInputType, std::string> nameToTypeMap({
		{ VertexInputType::Invalid, "Invalid" },
		{ VertexInputType::Position, "Position" },
		{ VertexInputType::Normal, "Normal" },
		{ VertexInputType::Binormal, "Binormal" },
		{ VertexInputType::Tangent, "Tangent" },
		{ VertexInputType::UV1, "UV1" },
		{ VertexInputType::UV2, "UV2" },
		{ VertexInputType::Color, "Color" },
    { VertexInputType::Joints, "Joints" },
    { VertexInputType::Weights, "Weights" },
		});

	if (nameToTypeMap.contains(t)) {
		return nameToTypeMap[t];
	}

	return nameToTypeMap[VertexInputType::Invalid];
}

void VertexSpec::SetInputAt(int index, VertexInput input) {
  uint64_t packed = ((uint64_t)input.type << 4) | (input.length & 0xF);
	this->hash |= (packed << (index * 8));
}

VertexSpec::VertexSpec(std::initializer_list<VertexInput> inputs) {
	int index = 0;
	for (auto i : inputs) {
		SetInputAt(index++, i);
	}
}

VertexSpec::VertexSpec(std::vector<VertexInput> inputs) {
	int index = 0;
	for (auto i : inputs) {
		SetInputAt(index++, i);
	}
}

VertexSpec::VertexSpec(const VertexSpec& other) :
hash(other.hash) { }

VertexSpec::VertexSpec(uint64_t hash) :
hash(hash) { }

VertexSpec::VertexSpec() :
hash(0) { }

std::vector<VertexInput> VertexSpec::GetInputs() const {
  std::vector<VertexInput> result;

	for (int i = 0; i < 8; i++) {
		uint64_t packed = (this->hash >> (i * 8)) & 0xFF;
		uint8_t type = packed >> 4;
		uint8_t length = packed & 0xF;

		if (type == (uint8_t) VertexInputType::Invalid) {
			break;
		}
		result.push_back({(VertexInputType)type, length});
	}

	return result;
}

int VertexSpec::GetLengthOf(VertexInputType input) const {
  for (int i = 0; i < 8; i++) {
		uint64_t packed = (this->hash >> (i * 8)) & 0xFF;
		uint8_t type = packed >> 4;
		uint8_t length = packed & 0xF;

		if (type == (uint8_t) input) {
			return length;
		}
		if (type == (uint8_t) VertexInputType::Invalid) {
			break;
		}
	}
	return 0;
}

unsigned int VertexSpec::VertexSize() const {
  unsigned int result = 0;

	for (int i = 0; i < 8; i++) {
		uint64_t packed = (this->hash >> (i * 8)) & 0xFF;
		uint8_t type = packed >> 4;
		uint8_t length = packed & 0xF;

		if (type == (uint8_t) VertexInputType::Invalid) {
			break;
		}
		result += length;
	}

	return result;
}

uint64_t VertexSpec::GetHash() const {
	return this->hash;
}

bool VertexSpec::Compatible(const VertexSpec& other) const {
  if (*this == other) {
		return true;
	}

	for (int i = 0; i < 8; i++) {
		uint64_t packed = (this->hash >> (i * 8)) & 0xFF;
		uint8_t type = packed >> 4;
		uint8_t length = packed & 0xF;

		if (type == (uint8_t) VertexInputType::Invalid) {
			break;
		}

		int otherLength = other.GetLengthOf((VertexInputType)type);
		if (otherLength < length) {
			return false;
		}
	}

	return true;
}

bool VertexSpec::operator==(const VertexSpec& other) const {
	return this->hash == other.hash;
}
bool VertexSpec::operator!=(const VertexSpec& other) const {
	return this->hash != other.hash;
}

const VertexSpec VertexSpec::Sprite {
	{ VertexInputType::Position, 2 },
	{ VertexInputType::UV1, 2 },
	{ VertexInputType::Color, 4 },
};

const VertexSpec VertexSpec::Point {
	{ VertexInputType::Position, 3 },
	{ VertexInputType::UV1, 2 },
	{ VertexInputType::Color, 4 },
};

const VertexSpec VertexSpec::Line {
	{ VertexInputType::Position, 3 },
	{ VertexInputType::UV1, 2 },
	{ VertexInputType::Color, 4 },
};

const VertexSpec VertexSpec::Mesh {
	{ VertexInputType::Position, 3 },
	{ VertexInputType::Normal, 3},
	{ VertexInputType::Tangent, 3},
	{ VertexInputType::UV1, 2 },
};

const VertexSpec VertexSpec::MeshColor {
	{ VertexInputType::Position, 3 },
	{ VertexInputType::Normal, 3},
	{ VertexInputType::UV1, 2 },
	{ VertexInputType::Color, 4 },
};

const VertexSpec VertexSpec::MeshFull {
	{ VertexInputType::Position, 3 },
	{ VertexInputType::Normal, 3},
	{ VertexInputType::Binormal, 3},
	{ VertexInputType::Tangent, 4},
	{ VertexInputType::UV1, 2 },
	{ VertexInputType::UV2, 2 },
	{ VertexInputType::Color, 4 },
};

const VertexSpec VertexSpec::MeshSkinned {
  { VertexInputType::Position, 3 },
  { VertexInputType::Normal, 3 },
  { VertexInputType::Tangent, 4 },
  { VertexInputType::UV1, 2 },
  { VertexInputType::Color, 4 },
  { VertexInputType::Joints, 4 },
  { VertexInputType::Weights, 4 },
};
