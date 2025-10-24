#pragma once

#include <filesystem>

#include <glad/glad.h>

#include <VertexSpec.h>
#include <Graphics.h>

namespace fs = std::filesystem;

class Mesh {
private:
	unsigned int meshID;
	VertexSpec meshSpec;

	Mesh(const unsigned int meshID, const VertexSpec& meshSpec);
public:
	Mesh();
	static const Mesh& Invalid;

	Mesh GetVariant(const VertexSpec& variantSpec) const;

	Mesh Clone(bool full = false);

	GLuint GetHandle() const;
	unsigned int GetVertexCount() const;
	unsigned int GetTriangleCount() const;

	const VertexSpec& GetMeshSpec() const;

	bool operator==(const Mesh& other);
	bool operator!=(const Mesh& other);

	static Mesh Load(fs::path modelPath, const VertexSpec& meshSpec);
	static Mesh Create(unsigned int vertexCount, float* vertexData, unsigned int triangleCount, unsigned int* indexData, const VertexSpec& meshSpec);
};