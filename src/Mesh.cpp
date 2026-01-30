#include <Mesh.h>

#include <vector>
#include <map>

#include "assimp/Importer.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include <Formatters.h>
#include <spdlog/spdlog.h>
#include <Material.h>
#include <Resources.h>

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

// Mesh::SubMesh::SubMesh():
// vertexCount(0),
// vertexData(0),
// vertexStride(0),
// faceCount(0),
// indexData(0),
// type(Mesh::MeshType::Points),
// materialIndex(0),
// indexBuffer(0) { }

void ReadVertex(const aiMesh* mesh, unsigned int vertexIndex, float* vertexData, const VertexSpec& spec) {
	memset(vertexData, 0, spec.VertexSize() * sizeof(float));

	glm::vec4* dataPointer = reinterpret_cast<glm::vec4*>(vertexData);

	if (mesh->HasPositions() && spec.GetLengthOf(VertexInputType::Position)) {
		*dataPointer = GetVec(mesh->mVertices[vertexIndex]);
	}
	dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + spec.GetLengthOf(VertexInputType::Position));

	if (mesh->HasNormals() && spec.GetLengthOf(VertexInputType::Normal)) {
		*dataPointer = GetVec(mesh->mNormals[vertexIndex]);			
	}
	dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + spec.GetLengthOf(VertexInputType::Normal));

	if (mesh->HasTangentsAndBitangents() && spec.GetLengthOf(VertexInputType::Binormal)) {
		*dataPointer = GetVec(mesh->mBitangents[vertexIndex]);
	}
	dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + spec.GetLengthOf(VertexInputType::Binormal));

	if (mesh->HasTangentsAndBitangents() && spec.GetLengthOf(VertexInputType::Tangent)) {
		*dataPointer = GetVec(mesh->mTangents[vertexIndex]);
	}
	dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + spec.GetLengthOf(VertexInputType::Tangent));

	if (mesh->HasTextureCoords(0) && spec.GetLengthOf(VertexInputType::UV1)) {
		*dataPointer = GetVec(mesh->mTextureCoords[0][vertexIndex]);
	}
	dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + spec.GetLengthOf(VertexInputType::UV1));

	if (mesh->HasTextureCoords(1) && spec.GetLengthOf(VertexInputType::UV2)) {
		*dataPointer = GetVec(mesh->mTextureCoords[0][vertexIndex]);
	}
	dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + spec.GetLengthOf(VertexInputType::UV2));
	
	if (mesh->HasVertexColors(0) && spec.GetLengthOf(VertexInputType::Color)) {
		*dataPointer = GetVec(mesh->mTextureCoords[0][vertexIndex]);
	}
	dataPointer = reinterpret_cast<glm::vec4*>(reinterpret_cast<float*>(dataPointer) + spec.GetLengthOf(VertexInputType::Color));
}

Mesh::MeshType Mesh::SubMesh::GetType() const {
	return this->type;
}

unsigned int Mesh::SubMesh::GetVerticesPerFace() const {
	return (unsigned int) this->type;
}

GLenum Mesh::SubMesh::GetDrawMode() const {
	static GLenum dict[] {
		GL_POINTS,
		GL_LINES,
		GL_TRIANGLES
	};

	return dict[GetVerticesPerFace() - 1];
}

int Mesh::SubMesh::GetMaterialIndex() const {
	return this->materialIndex;
}

GLuint Mesh::SubMesh::GetVertexArrayHandle() const {
	return this->handle.vertexArray;
}

GLuint Mesh::SubMesh::GetIndexBufferHandle() const {
	return this->handle.indexBuffer;
}

BoundingBox Mesh::SubMesh::GetBounds() const {
	return this->bounds;
}

unsigned int Mesh::SubMesh::GetVertexCount() const {
	return this->faceCount * GetVerticesPerFace();
}

unsigned int Mesh::SubMesh::GetFaceCount() const {
	return this->faceCount;
}

unsigned int Mesh::GetMaterialsCount() const {
	return this->materialCount;
}

std::vector<Material*> Mesh::GetDefaultMaterials() const {
	return this->materials;
}

unsigned int Mesh::GetSubMeshCount() const {
	return this->subMeshes.size();
}

std::vector<Mesh::SubMesh> Mesh::GetSubMeshes() const {
	return this->subMeshes;
}

const Mesh::SubMesh& Mesh::SubMeshAt(unsigned int index) const {
	return this->subMeshes.at(index);
}

const Mesh::SubMesh& Mesh::operator[](unsigned int index) const {
	return SubMeshAt(index);
}

Mesh* Mesh::Load(fs::path modelPath, bool loadMaterials) {
	if (!fs::exists(modelPath) || !fs::is_regular_file(modelPath)) {
		return nullptr;
	}

	Assimp::Importer importer{};

	const aiScene* loaded_scene = importer.ReadFile(modelPath.string(), 
		aiProcess_Triangulate | aiProcess_CalcTangentSpace
	);

	if (!loaded_scene || !loaded_scene->HasMeshes()) {
		return nullptr;
	}

	spdlog::info("Loading mesh {}", modelPath.string());
	
	// Three submeshes for each material:
	// - One with points
	// - One with lines
	// - One with triangles
	// SubMesh subMeshes[loaded_scene->mNumMaterials * 3];
	std::vector<SubMesh> subMeshes(loaded_scene->mNumMaterials * 3);

	for (int subMeshIndex = 0; subMeshIndex < loaded_scene->mNumMaterials * 3; subMeshIndex++) {
		subMeshes[subMeshIndex].type = MeshType((subMeshIndex % 3) + 1);
		subMeshes[subMeshIndex].materialIndex = subMeshIndex / 3;
	}

	unsigned int vertexCount = 0;

	VertexSpec meshSpec = VertexSpec::Mesh;

	for (unsigned int meshIndex = 0; meshIndex < loaded_scene->mNumMeshes; meshIndex++) {
		const aiMesh* currentMesh = loaded_scene->mMeshes[meshIndex];

		bool hasPoints = (currentMesh->mPrimitiveTypes & aiPrimitiveType_POINT) != 0;
		bool hasLines = (currentMesh->mPrimitiveTypes & aiPrimitiveType_LINE) != 0;
		bool hasTriangles = (currentMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) != 0;

		bool isSimple = (hasPoints + hasLines + hasTriangles) <= 1;

		if (isSimple) {
			if (hasPoints) {
				SubMesh& pointsMesh = subMeshes[currentMesh->mMaterialIndex * 3];

				pointsMesh.faceCount += currentMesh->mNumFaces;
			}
			else if (hasLines) {
				SubMesh& linesMesh = subMeshes[currentMesh->mMaterialIndex * 3 + 1];

				linesMesh.faceCount += currentMesh->mNumFaces;
			}
			else if (hasTriangles) {
				SubMesh& trianglesMesh = subMeshes[currentMesh->mMaterialIndex * 3 + 2];

				trianglesMesh.faceCount += currentMesh->mNumFaces;
			}
		}
		else {
			spdlog::warn("{}/{}: Meshes with mixed primitive types are unsupported", modelPath.string(), currentMesh->mName.C_Str());
			
			continue;
		}

		vertexCount += currentMesh->mNumVertices;
	}

	int subMeshCount = 0;
	
	int materialRemap[loaded_scene->mNumMaterials];
	for (int i = 0; i < loaded_scene->mNumMaterials; i++) {
		materialRemap[i] = -1;
	}

	for (int i = 0; i < subMeshes.size(); i++) {
		if (subMeshes[i].faceCount) {
			subMeshCount++;

			subMeshes[i].indexData = new unsigned int[subMeshes[i].faceCount * (unsigned int) subMeshes[i].type];

			subMeshes[i].faceCount = 0;

			for (int materialIndex = 0; materialIndex < loaded_scene->mNumMaterials; materialIndex++) {
				if (materialRemap[materialIndex] == -1) {
					materialRemap[materialIndex] = subMeshes[i].materialIndex;
				}

				if (!loadMaterials) {
					if (materialRemap[materialIndex] == subMeshes[i].materialIndex) {
						subMeshes[i].materialIndex = materialIndex;
	
						break;
					}
				}
			}
		}
	}

	int materialsCount = 0;

	for (int i = 0; i < loaded_scene->mNumMaterials; i++) {
		materialsCount += materialRemap[i] >= 0;
	}

	float* vertexData = new float[vertexCount * meshSpec.VertexSize() + 3];
	int vertexPointer = 0;

	for (unsigned int meshIndex = 0; meshIndex < loaded_scene->mNumMeshes; meshIndex++) {
		const aiMesh* currentMesh = loaded_scene->mMeshes[meshIndex];

		bool hasPoints = (currentMesh->mPrimitiveTypes & aiPrimitiveType_POINT) != 0;
		bool hasLines = (currentMesh->mPrimitiveTypes & aiPrimitiveType_LINE) != 0;
		bool hasTriangles = (currentMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) != 0;

		bool isSimple = (hasPoints + hasLines + hasTriangles) == 1;

		if (!isSimple) {
			continue;
		}

		int primitiveN = hasLines + (2 * hasTriangles);

		SubMesh& targetSubMesh = subMeshes[currentMesh->mMaterialIndex * 3 + primitiveN];

		for (unsigned int faceIndex = 0; faceIndex < currentMesh->mNumFaces; faceIndex++) {
			unsigned int baseIndex = (targetSubMesh.faceCount + faceIndex) * (primitiveN + 1);
			targetSubMesh.indexData[baseIndex + 0] = currentMesh->mFaces[faceIndex].mIndices[0] + vertexPointer;

			if (primitiveN > 0) {
				targetSubMesh.indexData[baseIndex + 1] = currentMesh->mFaces[faceIndex].mIndices[1] + vertexPointer;
			}
			if (primitiveN > 1) {
				targetSubMesh.indexData[baseIndex + 2] = currentMesh->mFaces[faceIndex].mIndices[2] + vertexPointer;
			}
		}

		targetSubMesh.faceCount += currentMesh->mNumFaces;

		for (unsigned int vertexIndex = 0; vertexIndex < currentMesh->mNumVertices; vertexIndex++) {
			float* currentVertex = vertexData + vertexPointer * meshSpec.VertexSize();

			ReadVertex(currentMesh, vertexIndex, currentVertex, meshSpec);

			vertexPointer += 1;
		}
	}

	std::sort(subMeshes.begin(), subMeshes.end(), [](const SubMesh& a, const SubMesh& b) -> bool {
		return a.faceCount > b.faceCount;
	});

	subMeshes.resize(subMeshCount);

	for (SubMesh& subMesh : subMeshes) {
		glm::vec3 minCorner = glm::vec3(INFINITY);
		glm::vec3 maxCorner = glm::vec3(-INFINITY);

		for (int faceIndex = 0; faceIndex < subMesh.faceCount; faceIndex++) {
			int verticesPerFace = (int) subMesh.type;

			for (int i = 0; i < verticesPerFace; i++) {
				glm::vec3 vertex = *(glm::vec3*) (vertexData + meshSpec.VertexSize() * subMesh.indexData[faceIndex * verticesPerFace + i]);

				if (vertex.x < minCorner.x) {
					minCorner.x = vertex.x;
				}
				if (vertex.y < minCorner.y) {
					minCorner.y = vertex.y;
				}
				if (vertex.z < minCorner.z) {
					minCorner.z = vertex.z;
				}

				if (vertex.x > maxCorner.x) {
					maxCorner.x = vertex.x;
				}
				if (vertex.y > maxCorner.y) {
					maxCorner.y = vertex.y;
				}
				if (vertex.z > maxCorner.z) {
					maxCorner.z = vertex.z;
				}
			}
		}

		subMesh.bounds = BoundingBox(minCorner, maxCorner);
	}

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * meshSpec.VertexSize() * sizeof(float), vertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	for (int subMeshIndex = 0; subMeshIndex < subMeshCount; subMeshIndex++) {
		GLuint subMeshVertexArray, subMeshIndexBuffer;

		glGenVertexArrays(1, &subMeshVertexArray);
		glGenBuffers(1, &subMeshIndexBuffer);

		subMeshes[subMeshIndex].handle.vertexArray = subMeshVertexArray;
		subMeshes[subMeshIndex].handle.indexBuffer = subMeshIndexBuffer;

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

		glBindVertexArray(subMeshVertexArray);

		unsigned int attributeOffset = 0;
		for (int input = int(VertexInputType::Position) - 1; input < int(VertexInputType::Color); input++) {
			int length = meshSpec.GetLengthOf(VertexInputType(input + 1));

			if (length > 0) {
				glVertexAttribPointer(input, length, GL_FLOAT, false, VertexSpec::Mesh.VertexSize() * sizeof(float), (void*) (attributeOffset * sizeof(float)));
				glEnableVertexAttribArray(input);
				
				attributeOffset += length;
			}
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMeshIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, subMeshes[subMeshIndex].faceCount * (int) subMeshes[subMeshIndex].type * sizeof(unsigned int), subMeshes[subMeshIndex].indexData, GL_STATIC_DRAW);

		glBindVertexArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	std::vector<Material*> materials;

	if (loadMaterials && loaded_scene->HasMaterials()) {
		ShaderProgram* pbrProg = ShaderProgram::Build().WithVertexShader(
			Resources::Get<VertexShader>("./res/shaders/lit.vert")
		).WithPixelShader(
			Resources::Get<PixelShader>("./res/shaders/pbr.frag")
		).Link();

		for (int matIndex = 0; matIndex < loaded_scene->mNumMaterials; matIndex++) {
			auto meshMaterial = loaded_scene->mMaterials[matIndex];

			aiString colorTexturePath;
			aiString normalTexturePath;
			aiString armTexturePath;

			meshMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &colorTexturePath);
			meshMaterial->GetTexture(aiTextureType_NORMALS, 0, &normalTexturePath);
			meshMaterial->GetTexture(aiTextureType_METALNESS, 0, &armTexturePath);

			Texture2D* albedoTex =
				fs::is_regular_file(modelPath.parent_path() / colorTexturePath.C_Str())
				? Resources::Get<Texture2D>((modelPath.parent_path() / colorTexturePath.C_Str()), Texture::ColorTextureRGB)
				: Resources::Get<Texture2D>("./res/textures/default_color.png", Texture::ColorTextureRGB);

			Texture2D* normalTex =
				fs::is_regular_file(modelPath.parent_path() / normalTexturePath.C_Str())
				? Resources::Get<Texture2D>((modelPath.parent_path() / normalTexturePath.C_Str()), Texture::TechnicalMapXYZ)
				: Resources::Get<Texture2D>("./res/textures/default_norm.png", Texture::TechnicalMapXYZ);
			
			Texture2D* armTex =
				fs::is_regular_file(modelPath.parent_path() / armTexturePath.C_Str())
				? Resources::Get<Texture2D>((modelPath.parent_path() / armTexturePath.C_Str()), Texture::TechnicalMapXYZ)
				: Resources::Get<Texture2D>("./res/textures/default_arm.png", Texture::TechnicalMapXYZ);
			
			Material* materialResult = new Material(pbrProg);
			materialResult->SetValue("albedoMap", albedoTex);
			materialResult->SetValue("normalMap", normalTex);
			materialResult->SetValue("armMap", armTex);

			materials.push_back(materialResult);
		}

		spdlog::info("Prepared materials");
	}

	Mesh* loadedMesh = new Mesh();
	loadedMesh->subMeshes = subMeshes;
	loadedMesh->materialCount = materialsCount;
	loadedMesh->materials = materials;
	loadedMesh->vertexCount = vertexCount;
	loadedMesh->vertexStride = VertexSpec::Mesh.VertexSize();
	loadedMesh->vertexBuffer = vertexBuffer;

	return loadedMesh;
}