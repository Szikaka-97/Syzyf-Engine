#pragma once

#include <filesystem>

#include <glad/glad.h>

#include <VertexSpec.h>
#include <Graphics.h>

namespace fs = std::filesystem;

class Mesh {
public:
	enum class MeshType {
		Points = 1,
		Lines = 2,
		Triangles = 3
	};

	class SubMesh {
		friend class Mesh;
	private:
		unsigned int faceCount;
		unsigned int* indexData;
		MeshType type;
		int materialIndex;

		struct {
			GLuint vertexArray;
			GLuint indexBuffer;
		} handle;
	public:
		MeshType GetType() const;
		unsigned int GetVerticesPerFace() const;
		GLenum GetDrawMode() const;

		int GetMaterialIndex() const;

		GLuint GetVertexArrayHandle() const;
		GLuint GetIndexBufferHandle() const;

		unsigned int GetVertexCount() const;
		unsigned int GetFaceCount() const;
	};

	// class MeshPart {
	// 	friend class Mesh;
	// private:
	// 	std::string name;
	// 	glm::mat4 transform;

	// 	int vertexCount;
	// 	int vertexOffset;
	// 	int faceCount;
	// 	int faceOffset;
	// };
private:
	std::vector<SubMesh> subMeshes;
	// std::map<std::string, MeshPart> parts;
	
	int materialCount;
	unsigned int vertexCount;
	float* vertexData;
	unsigned int vertexStride;
	GLuint vertexBuffer;
public:
	Mesh() = default;

	int GetMaterialsCount() const;

	int GetSubMeshCount() const;
	std::vector<SubMesh> GetSubMeshes() const;

	const SubMesh& SubMeshAt(unsigned int index) const;
	const SubMesh& operator[](unsigned int index) const;

	// Mesh* Separate(std::string partName);

	static Mesh* Load(fs::path modelPath);
	// static Mesh* Create(unsigned int vertexCount, float* vertexData, unsigned int triangleCount, unsigned int* indexData, const VertexSpec& meshSpec);
};