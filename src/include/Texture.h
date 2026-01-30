#pragma once

#include <filesystem>
#include <concepts>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include "stb_image.h"

namespace fs = std::filesystem;

enum class TextureType {
	Texture2D,
	Cubemap
};

enum class TextureChannels {
	Grayscale = 0,
	RG = 1,
	RGB = 2,
	RGBA = 3,
	Depth = 4
};

enum class TextureColor {
	Linear,
	SRGB
};

enum class TextureFormat {
	Ubyte = 0,
	Uint,
	Float
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

class Texture {
protected:
	template<typename T>
	struct TextureInfoBit {
		T value;
		bool dirty;

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
public:
	static constexpr TextureParams ColorTextureRGB {.channels = TextureChannels::RGB, .colorSpace = TextureColor::SRGB, .format = TextureFormat::Ubyte, .minFilter = TextureFilter::LinearMipmapLinear};
	static constexpr TextureParams ColorTextureRGBA {TextureChannels::RGBA, TextureColor::SRGB, TextureFormat::Ubyte};
	static constexpr TextureParams TechnicalMapXYZ {TextureChannels::RGB, TextureColor::Linear, TextureFormat::Ubyte};
	static constexpr TextureParams TechnicalMapXYZW {TextureChannels::RGBA, TextureColor::Linear, TextureFormat::Ubyte};
	static constexpr TextureParams DepthBuffer {TextureChannels::Depth, TextureColor::Linear, TextureFormat::Float};
	static constexpr TextureParams HDRColorBuffer {TextureChannels::RGBA, TextureColor::Linear, TextureFormat::Float};
	static constexpr TextureParams LDRColorBuffer {TextureChannels::RGBA, TextureColor::Linear, TextureFormat::Ubyte};

	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex* Load(const fs::path& texturePath, const TextureParams& loadParams) = delete;

	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex Wrap(GLuint handle);

	virtual ~Texture();

	virtual constexpr TextureType GetType() const = 0;

	static GLenum CalcInternalFormat(const TextureParams& params);

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

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
};

class Texture2D : public Texture {
public:
	Texture2D() = default;
	Texture2D(unsigned int width, unsigned int height, const TextureParams& creationParams);
	Texture2D(unsigned int width, unsigned int height, const TextureParams& creationParams, GLuint handle);

	static Texture2D* Load(const fs::path& texturePath, const TextureParams& loadParams);

	virtual constexpr TextureType GetType() const {
		return TextureType::Texture2D;
	}
};

class Cubemap : public Texture {
private:
	TextureInfoBit<TextureWrap> wrapW;
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
