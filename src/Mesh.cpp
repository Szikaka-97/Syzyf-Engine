#include <Mesh.h>

#include <vector>
#include <map>

#include "assimp/Importer.hpp"
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include <Formatters.h>
#include <spdlog/spdlog.h>

struct MeshInternal {
	fs::path sourceFile;
	unsigned int vertexCount;
	float* vertexData;
	unsigned int triangleCount;
	unsigned int* indexData;
	GLuint handle;
};

std::vector<std::map<uint64_t, MeshInternal>> loadedMeshes;

inline glm::vec4 GetVec(aiVector3f source) {
	return glm::vec4(
		source.x,
		source.y,
		source.z,
		0.0f
	);
}

inline glm::vec4 GetVec(aiColor4D source) {
	return glm::vec4(
		source.r,
		source.g,
		source.b,
		source.a
	);
}

Mesh::Mesh():
meshID(UINT_MAX),
meshSpec(0) { }

Mesh::Mesh(const unsigned int meshID, const VertexSpec& meshSpec):
meshID(meshID),
meshSpec(meshSpec) { }

const Mesh& Mesh::Invalid = {UINT_MAX, {}};

GLuint Mesh::GetHandle() const {
	if (this->meshID < UINT_MAX) {
		return loadedMeshes[this->meshID][this->meshSpec.GetHash()].handle;
	}
	return 0;
}

unsigned int Mesh::GetVertexCount() const {
	if (this->meshID < UINT_MAX) {
		return loadedMeshes[this->meshID][this->meshSpec.GetHash()].vertexCount;
	}
	return 0;
}
unsigned int Mesh::GetTriangleCount() const {
	if (this->meshID < UINT_MAX) {
		return loadedMeshes[this->meshID][this->meshSpec.GetHash()].triangleCount;
	}
	return 0;
}

const VertexSpec& Mesh::GetMeshSpec() const {
	return this->meshSpec;
}

bool Mesh::operator==(const Mesh& other) {
	return this->meshID == other.meshID;
}
bool Mesh::operator!=(const Mesh& other) {
	return this->meshID != other.meshID;
}

Mesh Mesh::Load(fs::path modelPath, const VertexSpec& meshSpec) {
	if (!fs::exists(modelPath) || !fs::is_regular_file(modelPath)) {
		return Mesh::Invalid;
	}

	unsigned int meshID = loadedMeshes.size();

	Assimp::Importer importer{};

	const aiScene* loaded_scene = importer.ReadFile(modelPath.string(), 
		aiProcess_Triangulate
	);

	if (!loaded_scene || !loaded_scene->HasMeshes()) {
		return Mesh::Invalid;
	}
	
	const aiMesh* currentMesh = loaded_scene->mMeshes[0];

	int vertexSize = meshSpec.VertexSize();
	unsigned int vertexCount = currentMesh->mNumVertices;
	unsigned int triangleCount = currentMesh->mNumFaces;

	float* vertexData = new float[vertexCount * vertexSize + 3];
	uint* indexData = new uint[triangleCount * 3];

	spdlog::info("Vertex size = {}", vertexSize);

	for (int vertIndex = 0; vertIndex < vertexCount; vertIndex++) {
		float* currentVertexData = vertexData + vertIndex * vertexSize;
		memset(currentVertexData, 0, vertexSize * sizeof(float));

		glm::vec4* dataPointer = reinterpret_cast<glm::vec4*>(currentVertexData);

		if (currentMesh->HasPositions() && meshSpec.GetLengthOf(VertexInputType::Position)) {
			*dataPointer = GetVec(currentMesh->mVertices[vertIndex]);
		}
		dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + meshSpec.GetLengthOf(VertexInputType::Position));

		if (currentMesh->HasNormals() && meshSpec.GetLengthOf(VertexInputType::Normal)) {
			*dataPointer = GetVec(currentMesh->mNormals[vertIndex]);			
		}
		dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + meshSpec.GetLengthOf(VertexInputType::Normal));

		if (currentMesh->HasTangentsAndBitangents() && meshSpec.GetLengthOf(VertexInputType::Binormal)) {
			*dataPointer = GetVec(currentMesh->mBitangents[vertIndex]);
		}
		dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + meshSpec.GetLengthOf(VertexInputType::Binormal));

		if (currentMesh->HasTangentsAndBitangents() && meshSpec.GetLengthOf(VertexInputType::Tangent)) {
			*dataPointer = GetVec(currentMesh->mTangents[vertIndex]);
		}
		dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + meshSpec.GetLengthOf(VertexInputType::Tangent));

		if (currentMesh->HasTextureCoords(0) && meshSpec.GetLengthOf(VertexInputType::UV1)) {
			*dataPointer = GetVec(currentMesh->mTextureCoords[0][vertIndex]);
		}
		dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + meshSpec.GetLengthOf(VertexInputType::UV1));

		if (currentMesh->HasTextureCoords(1) && meshSpec.GetLengthOf(VertexInputType::UV2)) {
			*dataPointer = GetVec(currentMesh->mTextureCoords[0][vertIndex]);
		}
		dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + meshSpec.GetLengthOf(VertexInputType::UV2));
		
		if (currentMesh->HasVertexColors(0) && meshSpec.GetLengthOf(VertexInputType::Color)) {
			*dataPointer = GetVec(currentMesh->mTextureCoords[0][vertIndex]);
		}
		dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + meshSpec.GetLengthOf(VertexInputType::Color));
	}

	for (int faceIndex = 0; faceIndex < triangleCount; faceIndex++) {
		indexData[faceIndex * 3 + 0] = currentMesh->mFaces[faceIndex].mIndices[0];
		indexData[faceIndex * 3 + 1] = currentMesh->mFaces[faceIndex].mIndices[1];
		indexData[faceIndex * 3 + 2] = currentMesh->mFaces[faceIndex].mIndices[2];
	}

	GLuint modelVAO;
	GLuint modelVertexBuffer, modelIndexBuffer;

	glGenVertexArrays(1, &modelVAO);
	glGenBuffers(1, &modelVertexBuffer);
	glGenBuffers(1, &modelIndexBuffer);

	glBindVertexArray(modelVAO);
	glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * vertexSize * sizeof(float), vertexData, GL_STATIC_DRAW);

	uint offset = 0;
	for (int input = int(VertexInputType::Position) - 1; input < int(VertexInputType::Color); input++) {
		int length = meshSpec.GetLengthOf(VertexInputType(input + 1));

		if (length > 0) {
			glVertexAttribPointer(input, length, GL_FLOAT, false, vertexSize * sizeof(float), (void*) (offset * sizeof(float)));
			glEnableVertexAttribArray(input);
			
			spdlog::info("Offset for input {} = {}", input, offset);
			
			offset += length;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleCount * 3 * sizeof(uint), indexData, GL_STATIC_DRAW);
	glBindBuffer(GL_INDEX_ARRAY, 0);
	
	glBindVertexArray(0);

	MeshInternal loadedMesh {
		modelPath,
		vertexCount,
		vertexData,
		triangleCount,
		indexData,
		modelVAO
	};

	loadedMeshes.push_back({
		{
			meshSpec.GetHash(),
			loadedMesh
		}
	});

	return {meshID, meshSpec};
}