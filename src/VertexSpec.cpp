#include <VertexSpec.h>

#include <algorithm>
#include <map>

void VertexSpec::SetInputAt(int index, VertexInput input) {
	uint64_t inputHash = input.length + 0b1000;

	this->hash |= (inputHash << ((uint64_t) input.type - 1u) * 4u);
	this->hash |= ((uint64_t) input.type) << (32u + index * 4);
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
		uint64_t type = ((this->hash >> (32u + i * 4)) & 0xf);

		if (type == (uint64_t) VertexInputType::Invalid) {
			break;
		}

		uint8_t count = (uint8_t) (this->hash >> (type - 1u) * 4) & 0xf;

		result.push_back({(VertexInputType) type, count});
	}

	return result;
}

int VertexSpec::GetLengthOf(VertexInputType input) const {
	return (this->hash >> (((uint64_t) input - 1) * 4)) & 0b111;
}

unsigned int VertexSpec::VertexSize() const {
	unsigned int result = 0;

	for (int i = 0; i < 8; i++) {
		uint64_t type = ((this->hash >> (32u + i * 4)) & 0xf);

		if (type == (uint64_t) VertexInputType::Invalid) {
			break;
		}

		result += (this->hash >> (type - 1u) * 4) & 0xf;
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
		uint64_t type = ((this->hash >> (32u + i * 4)) & 0xf);

		if (type == (uint64_t) VertexInputType::Invalid) {
			break;
		}

		unsigned int otherCount = other.hash >> ((type - 1u) * 4u) & 0b111;
		unsigned int thisCount = this->hash >> ((type - 1u) * 4u) & 0b111;

		if (otherCount < thisCount) {
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
	{ VertexInputType::Tangent, 3},
	{ VertexInputType::UV1, 2 },
	{ VertexInputType::UV2, 2 },
	{ VertexInputType::Color, 4 },
};