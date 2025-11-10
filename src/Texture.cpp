#define STB_IMAGE_IMPLEMENTATION

#include <Texture.h>

bool WrapModeValid(GLenum wrapMode) {
	return (
		wrapMode == GL_REPEAT
		||
		wrapMode == GL_MIRRORED_REPEAT
		||
		wrapMode == GL_CLAMP_TO_EDGE
	);
}

bool MinFilterValid(GLenum minFilter) {
	return (
		minFilter == GL_NEAREST
		||
		minFilter == GL_LINEAR
		||
		minFilter == GL_NEAREST_MIPMAP_NEAREST
		||
		minFilter == GL_LINEAR_MIPMAP_NEAREST
		||
		minFilter == GL_NEAREST_MIPMAP_LINEAR
		||
		minFilter == GL_LINEAR_MIPMAP_LINEAR
	);
}

bool MagFilterValid(GLenum magFilter) {
	return (
		magFilter == GL_NEAREST
		||
		magFilter == GL_LINEAR
	);
}

Texture::~Texture() {
	glDeleteTextures(1, &this->handle);
}

GLenum Texture::TextureFormatToGL(TextureFormat format) {
	static GLenum glFormats[] {
		GL_RED,
		GL_RG,
		GL_RGB,
		GL_RGBA,
		GL_R32F,
		GL_RG32F,
		GL_RGB32F,
		GL_RGBA32F,
	};

	if ((int) format > 0 && (int) format < sizeof(glFormats) / sizeof(GLenum)) {
		return glFormats[(int) format];
	}

	return GL_RED;
}

GLuint Texture::GetHandle() {
	return this->handle;
}

TextureFormat Texture::GetFormat() {
	return this->format;
}

GLenum Texture::GetWrapModeU() const {
	return this->wrapU.value;
}
void Texture::SetWrapModeU(GLenum wrapMode) {
	if (this->wrapU.value != wrapMode && WrapModeValid(wrapMode)) {
		this->wrapU.value = wrapMode;
		
		this->wrapU.dirty = true;
		this->dirty = true;
	}
}
GLenum Texture::GetWrapModeV() const {
	return this->wrapV.value;
}
void Texture::SetWrapModeV(GLenum wrapMode) {
	if (this->wrapV.value != wrapMode && WrapModeValid(wrapMode)) {
		this->wrapV.value = wrapMode;
		
		this->wrapV.dirty = true;
		this->dirty = true;
	}
}

GLenum Texture::GetMinFilter() const {
	return this->minFilter.value;
}
void Texture::SetMinFilter(GLenum minFilter) {
	if (this->minFilter.value != minFilter && MinFilterValid(minFilter)) {
		this->minFilter.value = minFilter;
		
		this->minFilter.dirty = true;
		this->dirty = true;
		
		if (minFilter >= GL_NEAREST_MIPMAP_NEAREST) {
			this->GenerateMipmaps();
		}
	}
}
GLenum Texture::GetMagFilter() const {
	return this->magFilter.value;
}
void Texture::SetMagFilter(GLenum magFilter) {
	if (this->magFilter.value != magFilter && MagFilterValid(magFilter)) {
		this->magFilter.value = magFilter;
		
		this->magFilter.dirty = true;
		this->dirty = true;
	}
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
		glTexParameteri(type, GL_TEXTURE_WRAP_S, this->wrapU.value);

		this->wrapU.dirty = false;
	}
	if (this->wrapV.dirty) {
		glTexParameteri(type, GL_TEXTURE_WRAP_T, this->wrapV.value);

		this->wrapV.dirty = false;
	}
	if (this->minFilter.dirty) {
		glTexParameteri(type, GL_TEXTURE_MIN_FILTER, this->minFilter.value);

		this->minFilter.dirty = false;
	}
	if (this->magFilter.dirty) {
		glTexParameteri(type, GL_TEXTURE_MAG_FILTER, this->magFilter.value);

		this->magFilter.dirty = false;
	}

	this->dirty = false;

	glBindTexture(type, 0);
}

template<> Texture2D* Texture::Load<Texture2D>(fs::path texturePath, TextureFormat format) {
	stbi_set_flip_vertically_on_load(true);
	
	fs::directory_entry textureFile(texturePath);

	if (!textureFile.exists() || !textureFile.is_regular_file()) {
		spdlog::error("File {} doesn't exist", texturePath.string());
		return nullptr;
	}

	int width, height, nrChannels;
	unsigned char *textureData = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);

	if (!textureData) {
		spdlog::error("stbi_load failed on file {}", texturePath.string());
		return nullptr;
	}

	GLuint textureHandle;
	glGenTextures(1, &textureHandle);

	glBindTexture(GL_TEXTURE_2D, textureHandle);

	glTexImage2D(GL_TEXTURE_2D, 0, TextureFormatToGL(format), width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
	
	stbi_image_free(textureData);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	Texture2D* result = new Texture2D();
	result->handle = textureHandle;
	result->format = format;
	
	result->SetWrapModeU(GL_CLAMP_TO_EDGE);
	result->SetWrapModeV(GL_CLAMP_TO_EDGE);
	result->SetMinFilter(GL_LINEAR);
	result->SetMagFilter(GL_LINEAR);
	
	result->Update();

	return result;
}

template<> Cubemap* Texture::Load<Cubemap>(fs::path texturePath, TextureFormat format) {
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
		unsigned char* textureData = stbi_load(texturePaths[i].c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
	
		if (!textureData) {
			spdlog::error("stbi_load failed on file {}", texturePaths[i].string());
			
			glDeleteTextures(1, &textureHandle);

			return nullptr;
		}
	
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, TextureFormatToGL(format), width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData
		);
		
		stbi_image_free(textureData);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	Cubemap* result = new Cubemap();
	result->handle = textureHandle;
	result->format = format;

	result->SetWrapModeU(GL_CLAMP_TO_EDGE);
	result->SetWrapModeV(GL_CLAMP_TO_EDGE);
	result->SetWrapModeW(GL_CLAMP_TO_EDGE);
	result->SetMinFilter(GL_LINEAR);
	result->SetMagFilter(GL_LINEAR);
	
	result->Update();

	return result;
}

Texture2D::Texture2D(unsigned int width, unsigned int height, TextureFormat format) {
	this->format = format;

	glGenTextures(1, &this->handle);

	glBindTexture(GL_TEXTURE_2D, this->handle);

	glTexImage2D(GL_TEXTURE_2D, 0, TextureFormatToGL(format), width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	this->SetWrapModeU(GL_CLAMP_TO_EDGE);
	this->SetWrapModeV(GL_CLAMP_TO_EDGE);
	this->SetMinFilter(GL_LINEAR);
	this->SetMagFilter(GL_LINEAR);

	this->Update();

	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D* Texture2D::Load(fs::path texturePath, TextureFormat format) {
	return Texture::Load<Texture2D>(texturePath, format);
}

Cubemap* Cubemap::Load(fs::path texturePath, TextureFormat format) {
	return Texture::Load<Cubemap>(texturePath, format);
}

GLenum Cubemap::GetWrapModeW() const {
	return this->wrapW.value;
}
void Cubemap::SetWrapModeW(GLenum wrapMode) {
	if (this->wrapW.value != wrapMode && WrapModeValid(wrapMode)) {
		this->wrapW.value = wrapMode;
		
		this->wrapW.dirty = true;
		this->dirty = true;
	}
}