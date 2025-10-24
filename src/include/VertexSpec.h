#pragma once

#include <vector>
#include <string>
#include <map>
#include <stdint.h>
#include <concepts>
#include <algorithm>

enum class VertexInputType : uint8_t {
	Invalid = 0,
	Position = 1,
	Normal,
	Binormal,
	Tangent,
	UV1,
	UV2,
	Color
};

struct VertexInput {
	VertexInputType type;
	uint8_t length;
};

class VertexSpec {
private:
	uint64_t hash;
	
	void SetInputAt(int index, VertexInput input);
public:
	constexpr static VertexInputType TypeFromSemantic(const std::string& s) {
		const static std::map<std::string, VertexInputType> nameToTypeMap({
			{ "POSITION", VertexInputType::Position },
			{ "NORMAL", VertexInputType::Normal },
			{ "BINORMAL", VertexInputType::Binormal },
			{ "TANGENT", VertexInputType::Tangent },
			{ "UV1", VertexInputType::UV1 },
			{ "UV2", VertexInputType::UV2 },
			{ "COLOR", VertexInputType::Color },
		});

		if (nameToTypeMap.contains(s)) {
			return nameToTypeMap.at(s);
		}

		return VertexInputType::Invalid;
	}
	constexpr static VertexInputType TypeFromName(const std::string& s) {
		const static std::map<std::string, VertexInputType> nameToTypeMap({
			{ "Position", VertexInputType::Position },
			{ "Normal", VertexInputType::Normal },
			{ "Binormal", VertexInputType::Binormal },
			{ "Tangent", VertexInputType::Tangent },
			{ "UV1", VertexInputType::UV1 },
			{ "UV2", VertexInputType::UV2 },
			{ "Color", VertexInputType::Color },
		});

		if (nameToTypeMap.contains(s)) {
			return nameToTypeMap.at(s);
		}

		return VertexInputType::Invalid;
	}
	constexpr static const std::string& TypeToName(VertexInputType t) {
		static std::map<VertexInputType, std::string> nameToTypeMap({
			{ VertexInputType::Invalid, "Invalid" },
			{ VertexInputType::Position, "Position" },
			{ VertexInputType::Normal, "Normal" },
			{ VertexInputType::Binormal, "Binormal" },
			{ VertexInputType::Tangent, "Tangent" },
			{ VertexInputType::UV1, "UV1" },
			{ VertexInputType::UV2, "UV2" },
			{ VertexInputType::Color, "Color" },
		});

		if (nameToTypeMap.contains(t)) {
			return nameToTypeMap[t];
		}

		return nameToTypeMap[VertexInputType::Invalid];
	}
	

	VertexSpec(std::initializer_list<VertexInput> inputs);
	VertexSpec(std::vector<VertexInput> inputs);

	VertexSpec(const VertexSpec& other);
	VertexSpec(uint64_t hash);
	VertexSpec();

	std::vector<VertexInput> GetInputs() const;

	int GetLengthOf(VertexInputType input) const;

	unsigned int VertexSize() const;
	uint64_t GetHash() const;

	bool Compatible(const VertexSpec& other) const;
	bool operator==(const VertexSpec& other) const;
	bool operator!=(const VertexSpec& other) const;

	const static VertexSpec Sprite;
	const static VertexSpec Mesh;
	const static VertexSpec MeshColor;
	const static VertexSpec MeshFull;
};
