#pragma once

#include <vector>
#include <string>
#include <stdint.h>

enum class VertexInputType : uint8_t {
	Invalid = 0,
	Position = 1,
	Normal,
	Binormal,
	Tangent,
	UV1,
	UV2,
	Color,
  Joints,
  Weights
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
	static VertexInputType TypeFromSemantic(const std::string& s);

	static VertexInputType TypeFromName(const std::string& s);

	static const std::string& TypeToName(VertexInputType t);

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

	const static VertexSpec Point;
	const static VertexSpec Line;
	const static VertexSpec Sprite;
	const static VertexSpec Mesh;
	const static VertexSpec MeshColor;
	const static VertexSpec MeshFull;
  const static VertexSpec MeshSkinned;
};
