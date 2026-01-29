#define STB_IMAGE_IMPLEMENTATION

#include <Texture.h>

#include <Material.h>
#include <Mesh.h>
#include <Resources.h>

GLenum ToGL(TextureWrap wrap) {
	static constexpr GLenum values[] {
		GL_CLAMP_TO_EDGE,
		GL_REPEAT,
		GL_MIRRORED_REPEAT
	};

	return values[(int) wrap];
}

GLenum ToGL(TextureFilter filter) {
	static constexpr GLenum values[] {
		GL_NEAREST,
		GL_LINEAR,
		GL_NONE,
		GL_NONE,
		GL_NEAREST_MIPMAP_NEAREST,
		GL_NEAREST_MIPMAP_LINEAR,
		GL_LINEAR_MIPMAP_NEAREST,
		GL_LINEAR_MIPMAP_LINEAR,
	};

	return values[(int) filter];
}

GLenum ToGL(TextureChannels channels) {
	static constexpr GLenum values[] {
		GL_RED,
		GL_RG,
		GL_RGB,
		GL_RGBA,
		GL_DEPTH_COMPONENT
	};

	return values[(int) channels];
}

GLenum ToGL(TextureFormat format) {
	static constexpr GLenum values[] {
		GL_UNSIGNED_BYTE,
		GL_UNSIGNED_INT,
		GL_FLOAT
	};

	return values[(int) format];
}

unsigned char* LoadTextureData(const fs::path& texPath, const TextureParams& loadParams, int* width, int* height, int* nrChannels) {
	unsigned char* textureData = nullptr;

	if (loadParams.format == TextureFormat::Float) {
		if (loadParams.colorSpace == TextureColor::Linear) {
			stbi_ldr_to_hdr_gamma(1.0f);
		}

		textureData = (unsigned char*) stbi_loadf(texPath.string().c_str(), width, height, nrChannels, loadParams.NumChannels());

		if (loadParams.colorSpace == TextureColor::Linear) {
			stbi_ldr_to_hdr_gamma(2.2f);
		}
	}
	else {
		textureData = stbi_load(texPath.string().c_str(), width, height, nrChannels, loadParams.NumChannels());
	}

	return textureData;
}

Texture::~Texture() {
	if (this->owning) {
		glDeleteTextures(1, &this->handle);
	}
}

GLenum Texture::CalcInternalFormat(const TextureParams& params) {
	int srgb = (int) (params.colorSpace == TextureColor::SRGB && params.format != TextureFormat::Float); // 2
	int numChannels = (int) params.channels; // 4
	int format = (int) params.format; // 3

	constexpr GLenum values[] {
		GL_R8,
		GL_R8,
		GL_RG8,
		GL_RG8,
		GL_RGB8,
		GL_SRGB8,
		GL_RGBA8,
		GL_SRGB8_ALPHA8,
		GL_R32UI,
		GL_R32UI,
		GL_RG32UI,
		GL_RG32UI,
		GL_RGB32UI,
		GL_RGB32UI,
		GL_RGBA32UI,
		GL_RGBA32UI,
		GL_R16F,
		GL_R16F,
		GL_RG16F,
		GL_RG16F,
		GL_RGB16F,
		GL_RGB16F,
		GL_RGBA16F,
		GL_RGBA16F,
	};
	
	int index = 4 * 2 * format + 2 * numChannels + srgb;

	GLenum result;

	if (index < 24) {
		result = values[index];
	}
	else {
		result = GL_DEPTH_COMPONENT;
	}

	return result;
}

unsigned int Texture::GetWidth() const {
	return this->width;
}
unsigned int Texture::GetHeight() const {
	return this->height;
}

GLuint Texture::GetHandle() {
	return this->handle;
}

TextureChannels Texture::GetChannels() const {
	return this->channels;
}
int Texture::GetNumChannels() const {
	return (int) this->channels + 1;
}
TextureColor Texture::GetColorSpace() const {
	return this->colorSpace;
}
TextureFormat Texture::GetFormat() const {
	return this->format;
}

TextureWrap Texture::GetWrapModeU() const {
	return this->wrapU.value;
}
void Texture::SetWrapModeU(TextureWrap wrapMode) {
	if (this->wrapU.value == wrapMode) {
		return;
	}

	this->wrapU.value = wrapMode;

	this->wrapU.dirty = true;
	this->dirty = true;
}
TextureWrap Texture::GetWrapModeV() const {
	return this->wrapV.value;
}
void Texture::SetWrapModeV(TextureWrap wrapMode) {
	if (this->wrapV.value == wrapMode) {
		return;
	}

	this->wrapV.value = wrapMode;
	
	this->wrapV.dirty = true;
	this->dirty = true;
}

TextureFilter Texture::GetMinFilter() const {
	return this->minFilter.value;
}
void Texture::SetMinFilter(TextureFilter minFilter) {
	if (this->minFilter.value == minFilter) {
		return;
	}

	this->minFilter.value = minFilter;
	
	this->minFilter.dirty = true;
	this->dirty = true;
	
	if ((int) minFilter & (int) TextureFilter::NearestMipmapNearest != 0) {
		this->GenerateMipmaps();
	}
}
TextureFilter Texture::GetMagFilter() const {
	return this->magFilter.value;
}
void Texture::SetMagFilter(TextureFilter magFilter) {
	if (this->magFilter.value == magFilter) {
		return;
	}

	this->magFilter.value = (TextureFilter) ((int) magFilter % 2);
	
	this->magFilter.dirty = true;
	this->dirty = true;
}

bool Texture::HasMipmaps() const {
	return this->mipmapped.value;
}
void Texture::GenerateMipmaps() {
	if (!this->mipmapped.value) {
		this->mipmapped.value = true;
		
		this->mipmapped.dirty = true;
		this->dirty = true;
	}
}

bool Texture::IsDirty() const {
	return this->dirty;
}
void Texture::Update() {
	GLenum glTexType[] {
		GL_TEXTURE_2D,
		GL_TEXTURE_CUBE_MAP
	};

	if (!this->dirty) {
		return;
	}

	GLenum type = glTexType[(int) this->GetType()];

	glBindTexture(type, this->handle);

	if (this->mipmapped.dirty) {
		glGenerateMipmap(type);

		this->mipmapped.dirty = false;
	}
	if (this->wrapU.dirty) {
		glTexParameteri(type, GL_TEXTURE_WRAP_S, ToGL(this->wrapU.value));

		this->wrapU.dirty = false;
	}
	if (this->wrapV.dirty) {
		glTexParameteri(type, GL_TEXTURE_WRAP_T, ToGL(this->wrapV.value));

		this->wrapV.dirty = false;
	}
	if (this->minFilter.dirty) {
		glTexParameteri(type, GL_TEXTURE_MIN_FILTER, ToGL(this->minFilter.value));

		this->minFilter.dirty = false;
	}
	if (this->magFilter.dirty) {
		glTexParameteri(type, GL_TEXTURE_MAG_FILTER, ToGL(this->magFilter.value));

		this->magFilter.dirty = false;
	}

	this->dirty = false;

	glBindTexture(type, 0);
}

template<> Texture2D* Texture::Load<Texture2D>(const fs::path& texturePath, const TextureParams& loadParams) {
	return Texture2D::Load(texturePath, loadParams);
}

template<> Cubemap* Texture::Load<Cubemap>(const fs::path& texturePath, const TextureParams& loadParams) {
	return Cubemap::Load(texturePath, loadParams);
}

Texture2D::Texture2D(unsigned int width, unsigned int height, const TextureParams& creationParams) {
	this->format = creationParams.format;
	this->channels = creationParams.channels;
	this->colorSpace = creationParams.colorSpace;
	this->owning = true;
	this->width = width;
	this->height = height;
	this->dirty = true;

	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	if (width > 0 && height > 0) {
		GLenum internalFormat = CalcInternalFormat(creationParams);
		GLenum format = ToGL(creationParams.channels);
		GLenum textureType = ToGL(creationParams.format);
		
		glCreateTextures(GL_TEXTURE_2D, 1, &this->handle);
		
		glBindTexture(GL_TEXTURE_2D, this->handle);
		
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, textureType, nullptr);

		this->Update();
	
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

Texture2D::Texture2D(unsigned int width, unsigned int height, const TextureParams& creationParams, GLuint handle) {
	this->format = creationParams.format;
	this->channels = creationParams.channels;
	this->colorSpace = creationParams.colorSpace;
	this->owning = true;
	this->handle = handle;
	this->width = width;
	this->height = height;
	this->dirty = true;

	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	glBindTexture(GL_TEXTURE_2D, this->handle);

	this->Update();

	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D* Texture2D::Load(const fs::path& texturePath, const TextureParams& loadParams) {
	stbi_set_flip_vertically_on_load(true);
	
	fs::directory_entry textureFile(texturePath);

	if (!textureFile.exists() || !textureFile.is_regular_file()) {
		spdlog::error("File {} doesn't exist", texturePath.string());
		return nullptr;
	}

	int width, height, nrChannels;
	unsigned char *textureData = LoadTextureData(texturePath, loadParams, &width, &height, &nrChannels);

	if (!textureData) {
		spdlog::error("stbi_load failed on file {}", texturePath.string());
		return nullptr;
	}

	GLuint textureHandle;
	glGenTextures(1, &textureHandle);

	glBindTexture(GL_TEXTURE_2D, textureHandle);

	GLenum internalFormat = CalcInternalFormat(loadParams);
	GLenum format = ToGL(loadParams.channels);
	GLenum textureType = ToGL(loadParams.format);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, textureType, textureData);
	
	stbi_image_free(textureData);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	Texture2D* result = new Texture2D(width, height, loadParams, textureHandle);

	return result;
}

Cubemap::Cubemap(unsigned int width, unsigned int height, const TextureParams& creationParams) {
	this->format = creationParams.format;
	this->channels = creationParams.channels;
	this->colorSpace = creationParams.colorSpace;
	this->owning = true;
	this->width = width;
	this->height = height;
	this->dirty = true;

	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetWrapModeW(creationParams.wrapW);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	if (width > 0 && height > 0) {
		GLenum internalFormat = CalcInternalFormat(creationParams);
		GLenum format = ToGL(creationParams.channels);
		GLenum textureType = ToGL(creationParams.format);
		
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &this->handle);
		
		glBindTexture(GL_TEXTURE_CUBE_MAP, this->handle);
		for (int i = 0; i < 6; i++) {
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
				internalFormat, width, height, 0, format, textureType, nullptr
			);
		}

		this->Update();
	
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
}

Cubemap::Cubemap(unsigned int width, unsigned int height, const TextureParams& creationParams, GLuint handle) {
	this->format = creationParams.format;
	this->channels = creationParams.channels;
	this->colorSpace = creationParams.colorSpace;
	this->owning = true;
	this->handle = handle;
	this->width = width;
	this->height = height;
	this->dirty = true;

	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetWrapModeW(creationParams.wrapW);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	glBindTexture(GL_TEXTURE_CUBE_MAP, this->handle);

	this->Update();

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Cubemap* Cubemap::Load(const fs::path& texturePath, const TextureParams& loadParams) {
	if (texturePath.extension().string() == ".hdr") {
		return Cubemap::LoadEquirectangular(texturePath, loadParams);
	}
	else {
		return Cubemap::LoadParts(texturePath, loadParams);
	}
}

Cubemap* Cubemap::LoadEquirectangular(const fs::path& texturePath, const TextureParams& loadParams) {
	static GLuint captureFBO = 0, captureRBO = 0;
	static ShaderProgram* cubemapBlitProg = nullptr;
	static Mesh* cubeMesh = nullptr;

	if (captureFBO == 0) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		cubemapBlitProg = ShaderProgram::Build()
		.WithVertexShader(Resources::Get<VertexShader>("./res/shaders/cubemapBlit/cubemapBlit.vert"))
		.WithPixelShader(Resources::Get<PixelShader>("./res/shaders/cubemapBlit/cubemapBlit.frag"))
		.Link();

		cubeMesh = Resources::Get<Mesh>("./res/models/not_cube.obj");
	}
	
	Texture2D* equTex = Texture2D::Load(texturePath, loadParams);
	
	if (equTex == nullptr) {
		return nullptr;
	}

	int texSize = equTex->GetHeight() / 3;

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, texSize, texSize);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	GLenum internalFormat = CalcInternalFormat(loadParams);
	GLenum format = ToGL(loadParams.channels);
	GLenum textureType = ToGL(loadParams.format);
	
	GLuint handle;

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle);

	glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
			internalFormat, texSize, texSize, 0, format, textureType, nullptr
		);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	glUseProgram(cubemapBlitProg->GetHandle());
	glUniform1i(glGetUniformLocation(cubemapBlitProg->GetHandle(), "equirectangularMap"), 0);
	glUniformMatrix4fv(glGetUniformLocation(cubemapBlitProg->GetHandle(), "projection"), 1, false, &captureProjection[0][0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, equTex->GetHandle());

	glViewport(0, 0, texSize, texSize);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	glCullFace(GL_FRONT);

	for (int face = 0; face < 6; face++) {
		glUniformMatrix4fv(glGetUniformLocation(cubemapBlitProg->GetHandle(), "view"), 1, false, &captureViews[face][0][0]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
							GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, handle, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(cubeMesh->SubMeshAt(0).GetVertexArrayHandle());
		glDrawElements(cubeMesh->SubMeshAt(0).GetDrawMode(), cubeMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}

	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	Cubemap* result = new Cubemap(texSize, texSize, loadParams, handle);
	
	delete equTex;

	return result;
}

Cubemap* Cubemap::LoadParts(const fs::path& texturePath, const TextureParams& loadParams) {
	stbi_set_flip_vertically_on_load(false);

	static std::string cubeSides[] {
		"_right",
		"_left",
		"_top",
		"_bottom",
		"_front",
		"_back"
	};

	fs::path texturePaths[6];
	
	for (int i = 0; i < 6; i++) {
		fs::path subTexturePath = texturePath;
		subTexturePath = texturePaths[i] = subTexturePath.replace_filename(texturePath.stem().string() + cubeSides[i] + texturePath.extension().string());

		fs::directory_entry textureFile(subTexturePath);
		
		if (!textureFile.exists() || !textureFile.is_regular_file()) {
			spdlog::error("File {} doesn't exist", subTexturePath.string());

			return nullptr;
		}
	}
	
	GLuint textureHandle;
	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureHandle);

	int width, height, nrChannels;
	for (int i = 0; i < 6; i++) {
		unsigned char *textureData = LoadTextureData(texturePaths[i], loadParams, &width, &height, &nrChannels);

		if (!textureData) {
			spdlog::error("stbi_load failed on file {}", texturePaths[i].string());
			
			glDeleteTextures(1, &textureHandle);

			return nullptr;
		}

		GLenum internalFormat = CalcInternalFormat(loadParams);
		GLenum format = ToGL(loadParams.channels);
		GLenum textureType = ToGL(loadParams.format);

		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, internalFormat, width, height, 0, format, textureType, textureData
		);
		
		stbi_image_free(textureData);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	Cubemap* result = new Cubemap();
	
	result->handle = textureHandle;
	result->format = loadParams.format;
	result->width = width;
	result->height = height;
	result->owning = true;

	result->SetWrapModeU(loadParams.wrapU);
	result->SetWrapModeV(loadParams.wrapV);
	result->SetWrapModeW(loadParams.wrapW);
	result->SetMinFilter(loadParams.minFilter);
	result->SetMagFilter(loadParams.magFilter);
	
	result->Update();

	return result;
}

Cubemap* Cubemap::GenerateIrradianceMap() {
	static GLuint captureFBO = 0, captureRBO = 0;
	static ShaderProgram* cubemapBlitProg = nullptr;
	static Mesh* cubeMesh = nullptr;

	if (captureFBO == 0) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		cubemapBlitProg = ShaderProgram::Build()
		.WithVertexShader(Resources::Get<VertexShader>("./res/shaders/cubemapBlit/cubemapBlit.vert"))
		.WithPixelShader(Resources::Get<PixelShader>("./res/shaders/cubemapBlit/cubemapIrradiance.frag"))
		.Link();

		cubeMesh = Resources::Get<Mesh>("./res/models/not_cube.obj");
	}

	TextureParams creationParams;
	creationParams.channels = this->channels;
	creationParams.format = this->format;
	creationParams.colorSpace = this->colorSpace;

	int texSize = 32;

	GLenum internalFormat = CalcInternalFormat(creationParams);
	GLenum format = ToGL(creationParams.channels);
	GLenum textureType = ToGL(creationParams.format);
	
	GLuint handle;

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle);

	glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
			internalFormat, texSize, texSize, 0, format, textureType, nullptr
		);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, texSize, texSize);

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	glUseProgram(cubemapBlitProg->GetHandle());
	glUniform1i(glGetUniformLocation(cubemapBlitProg->GetHandle(), "environmentMap"), 0);
	glUniformMatrix4fv(glGetUniformLocation(cubemapBlitProg->GetHandle(), "projection"), 1, false, &captureProjection[0][0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, this->GetHandle());

	glViewport(0, 0, texSize, texSize);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	glCullFace(GL_FRONT);

	for (int face = 0; face < 6; face++) {
		glUniformMatrix4fv(glGetUniformLocation(cubemapBlitProg->GetHandle(), "view"), 1, false, &captureViews[face][0][0]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
							GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, handle, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(cubeMesh->SubMeshAt(0).GetVertexArrayHandle());
		glDrawElements(cubeMesh->SubMeshAt(0).GetDrawMode(), cubeMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}

	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Cubemap* result = new Cubemap(width, height, creationParams, handle);
		
	result->SetWrapModeU(this->wrapU.value);
	result->SetWrapModeV(this->wrapV.value);
	result->SetWrapModeW(this->wrapW.value);
	result->SetMinFilter(this->minFilter.value);
	result->SetMagFilter(this->magFilter.value);
	
	result->Update();

	return result;
}

Cubemap* Cubemap::GeneratePrefilterIBLMap() {
	static GLuint captureFBO = 0, captureRBO = 0;
	static ShaderProgram* cubemapBlitProg = nullptr;
	static Mesh* cubeMesh = nullptr;

	if (captureFBO == 0) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		cubemapBlitProg = ShaderProgram::Build()
		.WithVertexShader(Resources::Get<VertexShader>("./res/shaders/cubemapBlit/cubemapBlit.vert"))
		.WithPixelShader(Resources::Get<PixelShader>("./res/shaders/cubemapBlit/cubemapPrefilter.frag"))
		.Link();

		cubeMesh = Resources::Get<Mesh>("./res/models/not_cube.obj");
	}

	TextureParams creationParams;
	creationParams.channels = this->channels;
	creationParams.format = this->format;
	creationParams.colorSpace = this->colorSpace;

	int texSize = 128;

	GLenum internalFormat = CalcInternalFormat(creationParams);
	GLenum format = ToGL(creationParams.channels);
	GLenum textureType = ToGL(creationParams.format);
	
	GLuint handle;

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle);

	glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
			internalFormat, texSize, texSize, 0, format, textureType, nullptr
		);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, texSize, texSize);

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	glUseProgram(cubemapBlitProg->GetHandle());
	glUniform1i(glGetUniformLocation(cubemapBlitProg->GetHandle(), "environmentMap"), 0);
	glUniformMatrix4fv(glGetUniformLocation(cubemapBlitProg->GetHandle(), "projection"), 1, false, &captureProjection[0][0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, this->GetHandle());

	glCullFace(GL_FRONT);

	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
		unsigned int mipWidth  = 128 * std::pow(0.5, mip);
		unsigned int mipHeight = 128 * std::pow(0.5, mip);

		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float) mip / (float) (maxMipLevels - 1);

		glUniform1f(glGetUniformLocation(cubemapBlitProg->GetHandle(), "roughness"), roughness);

		for (int face = 0; face < 6; face++) {
			glUniformMatrix4fv(glGetUniformLocation(cubemapBlitProg->GetHandle(), "view"), 1, false, &captureViews[face][0][0]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
								GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, handle, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(cubeMesh->SubMeshAt(0).GetVertexArrayHandle());
			glDrawElements(cubeMesh->SubMeshAt(0).GetDrawMode(), cubeMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}
	}

	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Cubemap* result = new Cubemap(width, height, creationParams, handle);
		
	result->SetWrapModeU(this->wrapU.value);
	result->SetWrapModeV(this->wrapV.value);
	result->SetWrapModeW(this->wrapW.value);
	result->SetMinFilter(TextureFilter::LinearMipmapLinear);
	result->SetMagFilter(this->magFilter.value);
	
	result->Update();

	return result;
}

Texture2D* Cubemap::GenerateBRDFConvolution() {
static GLuint captureFBO = 0, captureRBO = 0;
	static ShaderProgram* cubemapBlitProg = nullptr;
	static Mesh* planeMesh = nullptr;

	if (captureFBO == 0) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		cubemapBlitProg = ShaderProgram::Build()
		.WithVertexShader(Resources::Get<VertexShader>("./res/shaders/cubemapBlit/cubemapPrecomputeBRDF.vert"))
		.WithPixelShader(Resources::Get<PixelShader>("./res/shaders/cubemapBlit/cubemapPrecomputeBRDF.frag"))
		.Link();

		planeMesh = Resources::Get<Mesh>("./res/models/fullscreenquad.obj");
	}

	TextureParams creationParams;
	creationParams.channels = this->channels;
	creationParams.format = this->format;
	creationParams.colorSpace = this->colorSpace;

	int texSize = 512;

	GLenum internalFormat = CalcInternalFormat(creationParams);
	GLenum format = ToGL(creationParams.channels);
	GLenum textureType = ToGL(creationParams.format);
	
	GLuint handle;

	glCreateTextures(GL_TEXTURE_2D, 1, &handle);

	glBindTexture(GL_TEXTURE_2D, handle);
	
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texSize, texSize, 0, format, textureType, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, texSize, texSize);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, handle, 0);

	glViewport(0, 0, texSize, texSize);

	glUseProgram(cubemapBlitProg->GetHandle());

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(planeMesh->SubMeshAt(0).GetVertexArrayHandle());
	glDrawElements(planeMesh->SubMeshAt(0).GetDrawMode(), planeMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Texture2D* result = new Texture2D(width, height, creationParams, handle);

	result->SetWrapModeU(TextureWrap::Clamp);
	result->SetWrapModeV(TextureWrap::Clamp);
	result->SetMinFilter(TextureFilter::Linear);
	result->SetMagFilter(TextureFilter::Linear);
	
	result->Update();

	return result;
}

TextureWrap Cubemap::GetWrapModeW() const {
	return this->wrapW.value;
}
void Cubemap::SetWrapModeW(TextureWrap wrapMode) {
	if (this->wrapW.value == wrapMode) {
		return;
	}

	this->wrapW.value = wrapMode;
	
	this->wrapW.dirty = true;
	this->dirty = true;
}