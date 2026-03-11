#include <Texture.h>

#include "stb_image.h"

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
		GL_DEPTH_COMPONENT,
		GL_DEPTH_STENCIL
	};

	return values[(int) channels];
}

GLenum ToGL(TextureFormat format) {
	static constexpr GLenum values[] {
		GL_UNSIGNED_BYTE,
		GL_UNSIGNED_INT,
		GL_FLOAT,
		GL_UNSIGNED_INT_24_8
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
	return CalcInternalFormat(params.colorSpace, params.format, params.channels);
}

GLenum Texture::CalcInternalFormat(TextureColor colorSpace, TextureFormat format, TextureChannels channels) {
	int srgb = (int) (colorSpace == TextureColor::SRGB && format != TextureFormat::Float); // 2
	int numChannels = (int) channels; // 4

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
	
	int index = 4 * 2 * (int) format + 2 * numChannels + srgb;

	GLenum result;

	if (index < 24) {
		result = values[index];
	}
	else {
		if (channels == TextureChannels::Depth) {
			result = GL_DEPTH_COMPONENT;
		}
		else {
			result = GL_DEPTH24_STENCIL8;
		}
	}

	return result;
}

unsigned int Texture::GetWidth() const {
	return this->width;
}
unsigned int Texture::GetHeight() const {
	return this->height;
}
glm::uvec2 Texture::GetSize() const {
	return glm::uvec2(this->width, this->height);
}

void Texture::Resize(const glm::uvec2& newSize) {
	this->width = newSize.x;
	this->height = newSize.y;

	Create();
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
	
	if (((int) minFilter & (int) TextureFilter::NearestMipmapNearest) != 0) {
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

void Texture2D::Create() {
	if (this->width > 0 && this->height > 0) {
		GLenum internalFormat = CalcInternalFormat(this->colorSpace, this->format, this->channels);
		GLenum texFormat = ToGL(this->channels);
		GLenum textureType = ToGL(this->format);
		
		if (!this->handle) {
			glCreateTextures(GL_TEXTURE_2D, 1, &this->handle);
		}
		
		glBindTexture(GL_TEXTURE_2D, this->handle);
		
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, this->width, this->height, 0, texFormat, textureType, nullptr);

		glBindTexture(GL_TEXTURE_2D, 0);
		
		this->Update();
	}
}

Texture2D::Texture2D(unsigned int width, unsigned int height, const TextureParams& creationParams) {
	this->format = creationParams.format;
	this->channels = creationParams.channels;
	this->colorSpace = creationParams.colorSpace;
	this->owning = true;
	this->handle = 0;
	this->width = width;
	this->height = height;
	this->dirty = true;

	this->mipmapped = TextureInfoBit<bool>();
	this->wrapU = TextureInfoBit<TextureWrap>();
	this->wrapV = TextureInfoBit<TextureWrap>();
	this->minFilter = TextureInfoBit<TextureFilter>();
	this->magFilter = TextureInfoBit<TextureFilter>();

	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	Create();
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

	this->mipmapped = TextureInfoBit<bool>();
	this->wrapU = TextureInfoBit<TextureWrap>();
	this->wrapV = TextureInfoBit<TextureWrap>();
	this->minFilter = TextureInfoBit<TextureFilter>();
	this->magFilter = TextureInfoBit<TextureFilter>();

	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	this->Update();
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

void Cubemap::Create() {
	if (this->width > 0 && this->height > 0) {
		GLenum internalFormat = CalcInternalFormat(this->colorSpace, this->format, this->channels);
		GLenum texFormat = ToGL(this->channels);
		GLenum textureType = ToGL(this->format);
		
		if (!this->handle) {
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &this->handle);
		}
		
		glBindTexture(GL_TEXTURE_CUBE_MAP, this->handle);
		for (int i = 0; i < 6; i++) {
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
				internalFormat, this->width, this->height, 0, texFormat, textureType, nullptr
			);
		}

		this->Update();
	
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
}

Cubemap::Cubemap(unsigned int width, unsigned int height, const TextureParams& creationParams) {
	this->format = creationParams.format;
	this->channels = creationParams.channels;
	this->colorSpace = creationParams.colorSpace;
	this->owning = true;
	this->handle = 0;
	this->width = width;
	this->height = height;
	this->dirty = true;

	this->mipmapped = TextureInfoBit<bool>();
	this->wrapU = TextureInfoBit<TextureWrap>();
	this->wrapV = TextureInfoBit<TextureWrap>();
	this->wrapW = TextureInfoBit<TextureWrap>();
	this->minFilter = TextureInfoBit<TextureFilter>();
	this->magFilter = TextureInfoBit<TextureFilter>();
	
	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetWrapModeW(creationParams.wrapW);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	Create();
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

	this->mipmapped = TextureInfoBit<bool>();
	this->wrapU = TextureInfoBit<TextureWrap>();
	this->wrapV = TextureInfoBit<TextureWrap>();
	this->wrapW = TextureInfoBit<TextureWrap>();
	
	this->SetWrapModeU(creationParams.wrapU);
	this->SetWrapModeV(creationParams.wrapV);
	this->SetWrapModeW(creationParams.wrapW);
	this->SetMinFilter(creationParams.minFilter);
	this->SetMagFilter(creationParams.magFilter);

	this->Update();
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
	static ComputeShaderDispatch* cubemapBlitProg = new ComputeShaderDispatch(ResourceDatabase::Global->Get<ComputeShader>("./res/shaders/cubemapBlit/cubemapFromEqu.comp"));

	Texture2D* equTex = Texture2D::Load(texturePath, loadParams);
	
	if (equTex == nullptr) {
		return nullptr;
	}

	int texSize = equTex->GetHeight() / 3;

	TextureParams creationParams {
		.channels = TextureChannels::RGBA,
		.colorSpace = TextureColor::Linear,
		.format = TextureFormat::Float,
		.wrapU = loadParams.wrapU,
		.wrapV = loadParams.wrapV,
		.wrapW = loadParams.wrapW,
		.minFilter = loadParams.minFilter,
		.magFilter = loadParams.magFilter
	};

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

	Cubemap* result = new Cubemap(texSize, texSize, loadParams, handle);
	
	cubemapBlitProg->GetData()->SetValue("equirectangularMap", equTex);
	cubemapBlitProg->GetData()->SetValue("outputImg", result);

	cubemapBlitProg->Dispatch(std::ceil(texSize / 8.0f), std::ceil(texSize / 8.0f), 1);

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
	static ComputeShaderDispatch* irradianceProg = new ComputeShaderDispatch(ResourceDatabase::Global->Get<ComputeShader>("./res/shaders/cubemapBlit/cubemapIrradiance.comp"));

	TextureParams creationParams {
		.channels = TextureChannels::RGBA,
		.colorSpace = TextureColor::Linear,
		.format = TextureFormat::Float,
		.wrapU = TextureWrap::Repeat,
		.wrapV = TextureWrap::Repeat,
		.wrapW = TextureWrap::Repeat,
		.minFilter = TextureFilter::Linear,
		.magFilter = TextureFilter::Linear
	};

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

	Cubemap* result = new Cubemap(width, height, creationParams, handle);
	
	irradianceProg->GetData()->SetValue("environmentMap", this);
	irradianceProg->GetData()->SetValue("outputImg", result);

	irradianceProg->Dispatch(std::ceil(texSize / 8.0f), std::ceil(texSize / 8.0f), 1);

	result->SetMinFilter(TextureFilter::LinearMipmapLinear);
	result->Update();

	return result;
}

Cubemap* Cubemap::GeneratePrefilterIBLMap() {
	static ComputeShaderDispatch* cubemapPrefilterProg = new ComputeShaderDispatch(ResourceDatabase::Global->Get<ComputeShader>("./res/shaders/cubemapBlit/cubemapPrefilter.comp"));

	TextureParams creationParams {
		.channels = TextureChannels::RGBA,
		.colorSpace = TextureColor::Linear,
		.format = TextureFormat::Float,
		.wrapU = TextureWrap::Repeat,
		.wrapV = TextureWrap::Repeat,
		.wrapW = TextureWrap::Repeat,
		.minFilter = TextureFilter::LinearMipmapLinear,
		.magFilter = TextureFilter::Linear
	};
	
	constexpr int texSize = 128;
	constexpr unsigned int maxMipLevels = 5;

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

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, maxMipLevels - 1);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxMipLevels - 1);
	
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	
	Cubemap* result = new Cubemap(width, height, creationParams, handle);
	
	cubemapPrefilterProg->GetData()->SetValue("environmentMap", this);
	
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
		unsigned int mipSize  = 128 * std::pow(0.5, mip);
		
		float roughness = (float) mip / (float) (maxMipLevels - 1);

		cubemapPrefilterProg->GetData()->SetValue("outputImg", result, mip);
		cubemapPrefilterProg->GetData()->SetValue("roughness", roughness);

		cubemapPrefilterProg->Dispatch(std::ceil(mipSize / 8.0f), std::ceil(mipSize / 8.0f), 1);
	}

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
