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

enum class TextureFormat {
	Grayscale = 0,
	RG,
	RGB,
	RGBA,
	GrayscaleFloat,
	RGFloat,
	RGBFloat,
	RGBAFloat,
	RUInt,
	RGUInt,
	RGBUInt,
	RGBAUInt,
	Depth
};

class Texture {
protected:
	template<typename T>
	struct TextureInfoBit {
		T value;
		bool dirty;
	};

	GLuint handle;
	bool dirty;
	bool owning;

	TextureFormat format;

	TextureInfoBit<GLenum> wrapU;
	TextureInfoBit<GLenum> wrapV;
	TextureInfoBit<GLenum> minFilter;
	TextureInfoBit<GLenum> magFilter;
	TextureInfoBit<bool> mipmapped;
public:
	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex* Load(fs::path texturePath, TextureFormat format) = delete;

	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex Wrap(GLuint handle);

	template <class T_Tex>
		requires (std::derived_from<T_Tex, Texture>)
	static T_Tex Wrap(GLuint handle, TextureFormat format);

	virtual ~Texture();

	virtual constexpr TextureType GetType() const = 0;

	static GLenum TextureFormatToGL(TextureFormat format);

	GLuint GetHandle();
	TextureFormat GetFormat();

	GLenum GetWrapModeU() const;
	void SetWrapModeU(GLenum wrapMode);
	GLenum GetWrapModeV() const;
	void SetWrapModeV(GLenum wrapMode);

	GLenum GetMinFilter() const;
	void SetMinFilter(GLenum minFilter);
	GLenum GetMagFilter() const;
	void SetMagFilter(GLenum magFilter);

	bool HasMipmaps() const;
	void GenerateMipmaps();

	bool IsDirty() const;
	void Update();
};

class Texture2D : public Texture {
public:
	Texture2D() = default;
	Texture2D(unsigned int width, unsigned int height, TextureFormat format = TextureFormat::RGBA);
	Texture2D(GLuint handle, TextureFormat format, bool mipmapped);

	static Texture2D* Load(fs::path texturePath, TextureFormat format);

	virtual constexpr TextureType GetType() const {
		return TextureType::Texture2D;
	}
};

class Cubemap : public Texture {
private:
	TextureInfoBit<GLenum> wrapW;
public:
	Cubemap() = default;
	Cubemap(unsigned int width, unsigned int height, TextureFormat format = TextureFormat::RGBA);

	static Cubemap* Load(fs::path texturePath, TextureFormat format);

	GLenum GetWrapModeW() const;
	void SetWrapModeW(GLenum wrapMode);

	virtual constexpr TextureType GetType() const {
		return TextureType::Cubemap;
	}
};

template<> Texture2D* Texture::Load<Texture2D>(fs::path texturePath, TextureFormat format);
template<> Cubemap* Texture::Load<Cubemap>(fs::path texturePath, TextureFormat format);

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

