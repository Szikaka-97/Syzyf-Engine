#pragma once

#include <filesystem>
#include <concepts>

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <Resources.h>

namespace fs = std::filesystem;

enum class TextureType {
	Texture2D,
	Texture3D,
	Cubemap
};

enum class TextureChannels {
	Grayscale = 0,
	RG = 1,
	RGB = 2,
	RGBA = 3,
	Depth = 4,
	DepthStencil = 5,
	GrayscaleInteger = 6
};

enum class TextureColor {
	Linear,
	SRGB
};

enum class TextureFormat {
	Ubyte = 0,
	Uint,
	Float,
	Float8,
	Float32,
	PackedDepthStencil,
};

enum class TextureWrap {
	Clamp = 0,
	Repeat,
	MirrorRepeat
};

enum class TextureFilter {
	Nearest = 0,
	Linear,
	NearestMipmapNearest = 4,
	NearestMipmapLinear,
	LinearMipmapNearest,
	LinearMipmapLinear,
};

struct TextureParams {
	TextureChannels channels;
	TextureColor colorSpace;
	TextureFormat format;
	TextureWrap wrapU = TextureWrap::Repeat;
	TextureWrap wrapV = TextureWrap::Repeat;
	TextureWrap wrapW = TextureWrap::Repeat;
	TextureFilter minFilter = TextureFilter::Linear;
	TextureFilter magFilter = TextureFilter::Linear;

	// constexpr TextureParams(TextureChannels channels, TextureColor colorSpace, TextureFormat format):
	// channels(channels),
	// colorSpace(colorSpace),
	// format(format) { }

	constexpr unsigned int NumChannels() const {
		return (int) this->channels + 1;
	}
};

class Texture : public Resource {
protected:
	template<typename T>
	struct TextureInfoBit {
		T value = (T) 0;
		bool dirty = true;

		TextureInfoBit() = default;
	};

	unsigned int width;
	unsigned int height;

	GLuint handle;
	bool dirty;
	bool owning;

	TextureChannels channels;
	TextureColor colorSpace;
	TextureFormat format;

	TextureInfoBit<TextureWrap> wrapU;
	TextureInfoBit<TextureWrap> wrapV;
	TextureInfoBit<TextureFilter> minFilter;
	TextureInfoBit<TextureFilter> magFilter;
	TextureInfoBit<bool> mipmapped;

	fs::path path;

	virtual void Create() = 0;
public:
	static constexpr TextureParams ColorTextureRGB {.channels = TextureChannels::RGB, .colorSpace = TextureColor::SRGB, .format = TextureFormat::Ubyte, .minFilter = TextureFilter::LinearMipmapLinear};
	static constexpr TextureParams ColorTextureRGBA {TextureChannels::RGBA, TextureColor::SRGB, TextureFormat::Ubyte};
	static constexpr TextureParams TechnicalMapXYZ {TextureChannels::RGB, TextureColor::Linear, TextureFormat::Ubyte};
	static constexpr TextureParams TechnicalMapXYZW {TextureChannels::RGBA, TextureColor::Linear, TextureFormat::Ubyte};
	static constexpr TextureParams DepthBuffer {TextureChannels::Depth, TextureColor::Linear, TextureFormat::Float};
	static constexpr TextureParams DepthStencilBuffer {TextureChannels::DepthStencil, TextureColor::Linear, TextureFormat::PackedDepthStencil};
	static constexpr TextureParams HDRColorBuffer {TextureChannels::RGBA, TextureColor::Linear, TextureFormat::Float, TextureWrap::Clamp, TextureWrap::Clamp};
	static constexpr TextureParams LDRColorBuffer {TextureChannels::RGBA, TextureColor::Linear, TextureFormat::Ubyte, TextureWrap::Clamp, TextureWrap::Clamp};

	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex* Load(const fs::path& texturePath, const TextureParams& loadParams) = delete;

	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex* Load(const unsigned char* data, const int length, const TextureParams loadParams) = delete;

	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex Wrap(GLuint handle);

	virtual ~Texture();

	virtual constexpr TextureType GetType() const = 0;

	static GLenum CalcInternalFormat(const TextureParams& params);
	static GLenum CalcInternalFormat(TextureColor colorSpace, TextureFormat format, TextureChannels channels);

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;
	glm::uvec2 GetSize() const;

	void Resize(const glm::uvec2& newSize);

	GLuint GetHandle();
	TextureChannels GetChannels() const;
	int GetNumChannels() const;
	TextureColor GetColorSpace() const;
	TextureFormat GetFormat() const;

	TextureWrap GetWrapModeU() const;
	void SetWrapModeU(TextureWrap wrapMode);
	TextureWrap GetWrapModeV() const;
	void SetWrapModeV(TextureWrap wrapMode);

	TextureFilter GetMinFilter() const;
	void SetMinFilter(TextureFilter minFilter);
	TextureFilter GetMagFilter() const;
	void SetMagFilter(TextureFilter magFilter);

	bool HasMipmaps() const;
	void GenerateMipmaps();

	bool IsDirty() const;
	void Update();

	virtual fs::path GetPath() const;
	virtual uint64_t GetHash() const;
};

class Texture2D : public Texture {
protected:
	virtual void Create();
public:
	Texture2D() = default;
	Texture2D(unsigned int width, unsigned int height, const TextureParams& creationParams);
	Texture2D(unsigned int width, unsigned int height, const TextureParams& creationParams, GLuint handle);

	static Texture2D* Create(unsigned char* textureData, int width, int height, const TextureParams& loadParams);

	static Texture2D* Load(const fs::path& texturePath, const TextureParams& loadParams, bool flip = true);

	static Texture2D* Load(const unsigned char* data, const int length, const TextureParams loadParams, bool flip = true);

	virtual constexpr TextureType GetType() const {
		return TextureType::Texture2D;
	}
};

class Texture3D : public Texture {
private:
	unsigned int depth;
	TextureInfoBit<TextureWrap> wrapW;
protected:
	virtual void Create();
public:
	Texture3D() = default;
	Texture3D(unsigned int width, unsigned int height, unsigned int depth, const TextureParams& creationParams);
	Texture3D(unsigned int width, unsigned int height, unsigned int depth, const TextureParams& creationParams, GLuint handle);


  static Texture3D* Create(unsigned char* textureData, int width, int height, int depth, const TextureParams& loadParams);

  static Texture3D* Load(const unsigned char* data, const int length, const TextureParams loadParams);

	virtual constexpr TextureType GetType() const {
		return TextureType::Texture3D;
	}

	unsigned int GetDepth() const;
    glm::uvec3 GetSize3D() const;

	void Resize3D(const glm::uvec3& newSize);

	TextureWrap GetWrapModeW() const;
	void SetWrapModeW(TextureWrap wrapMode);
};

class Cubemap : public Texture {
private:
	TextureInfoBit<TextureWrap> wrapW;
protected:
	virtual void Create();
public:
	Cubemap() = default;
	Cubemap(unsigned int width, unsigned int height, const TextureParams& creationParams);
	Cubemap(unsigned int width, unsigned int height, const TextureParams& creationParams, GLuint handle);

	static Cubemap* Load(const fs::path& texturePath, const TextureParams& loadParams);
	static Cubemap* LoadParts(const fs::path& texturePath, const TextureParams& loadParams);
	static Cubemap* LoadEquirectangular(const fs::path& texturePath, const TextureParams& loadParams);

	Cubemap* GenerateIrradianceMap();
	Cubemap* GeneratePrefilterIBLMap();
	
	TextureWrap GetWrapModeW() const;
	void SetWrapModeW(TextureWrap wrapMode);

	virtual constexpr TextureType GetType() const {
		return TextureType::Cubemap;
	}
};

template<> Texture2D* Texture::Load<Texture2D>(const fs::path& texturePath, const TextureParams& loadParams);
template<> Texture2D* Texture::Load<Texture2D>(const unsigned char* data, const int length, const TextureParams loadParams);
template<> Texture3D* Texture::Load<Texture3D>(const unsigned char* data, const int length, const TextureParams loadParams);
template<> Cubemap* Texture::Load<Cubemap>(const fs::path& texturePath, const TextureParams& loadParams);

template <class T_Tex>
	requires (std::derived_from<T_Tex, Texture>)
T_Tex Texture::Wrap(GLuint handle) {
	T_Tex result;

	result.handle = handle;
	result.dirty = false;
	result.owning = false;

	// result.format = TextureFormat::RGUInt;

	return result;
}
