#pragma once
#include <devkit/gfx/common.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/attachment_base.h>
#include <devkit/gfx/api_resources.h>

namespace dk::gfx {

class FrameBuffer;
class TextureUnit;

enum class Channels {
	R, RG, RGB, BGR, RGBA, BGRA, Depth, DepthStencil
};

namespace api {
unsigned toUnderlying(Channels);
unsigned internalFormat(Channels);
}

class RenderTarget {
protected:
	virtual void setAsTarget(api::Attachment attachment, unsigned colorIndex = 0, unsigned level = 0) = 0;
	virtual glm::ivec3 targetSize() const = 0;
	virtual unsigned samples() const { return 1; }

	friend class FrameBuffer;
};

class Texture
	: public RenderTarget
{

public:
	enum class MinFilter { Nearest, Linear, LinearMipmapLinear, LinearMipmapNearest, NearestMipmapLinear, NearestMipmapNearest };
	enum class MagFilter { Nearest, Linear };

	class Config : DK_CONFIG_SPECIALIZATION(Texture,
		MinFilter, MagFilter);

	Config config;

public:
	Texture(api::TextureType type, Channels channels)
		: m_type(type)
		, m_channels(channels)
	{ }

	unsigned handle();

protected:
	using Initializer = std::optional<std::function<void(unsigned, Texture*)>>;

	api::TextureType m_type;
	api::Texture     m_apiHandle;
	Channels         m_channels;
	Initializer      m_initializer;

	void bindToUnit(unsigned unit);

	void updateConfig();

	template <typename P>
	friend void setTextureProperty(Texture&, const P&);

	friend class TextureUnit;
};

class Texture1D
	: public Texture
{
public:
	Texture1D(Texture1D&&)            = default;
	Texture1D& operator=(Texture1D&&) = default;

	//Texture1D()
	//	: Texture(api::TextureType::Unset, Channels::R)
	//{ }

	//Texture1D(int size, Channels channels)
	//	: Texture(api::TextureType::Texture1D, channels)
	//	, m_size(size)
	//{ }

private:
	int m_size = 0;

	void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

	glm::ivec3 targetSize() const override
	{ return { m_size, 1, 1 }; }
};

class Texture2D
	: public Texture
{
public:
	Texture2D(Texture2D&&)            = default;
	Texture2D& operator=(Texture2D&&) = default;

	Texture2D()
		: Texture(api::TextureType::Unset, Channels::R)
	{ }

	Texture2D(const glm::ivec2& size, Channels channels = Channels::RGBA);

	Texture2D(const std::filesystem::path& path);

	// @breif Call after graphics context was initalized
	static Texture2D loadFromFileAndInitialize(const std::string& path);

	glm::ivec2 size() const { return m_size; }

private:
	glm::ivec2 m_size;

	void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

	glm::ivec3 targetSize() const override
	{ return { m_size.x, m_size.y, 1 }; }
};

class Cubemap
	: public Texture
{
public:
	Cubemap(Cubemap&&)            = default;
	Cubemap& operator=(Cubemap&&) = default;

	Cubemap()
		: Texture(api::TextureType::Unset, Channels::R)
	{ }

	Cubemap(const std::array<std::filesystem::path, 6>& paths);

private:
	glm::ivec2 m_size;

	void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

	glm::ivec3 targetSize() const override
	{ return { m_size.x, m_size.y, 1 }; }
};

class MultisampledTexture2D
	: public Texture
{
public:
	MultisampledTexture2D(MultisampledTexture2D&&)            = default;
	MultisampledTexture2D& operator=(MultisampledTexture2D&&) = default;

	MultisampledTexture2D(const glm::ivec2& size, unsigned samples, Channels channels = Channels::RGBA);

private:
	glm::ivec2 m_size;
	unsigned   m_samples;

	void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

	glm::ivec3 targetSize() const override
	{ return { m_size.x, m_size.y, 1 }; }

	unsigned samples() const override { return m_samples; }
};


//class FrameBuffer {
//private:
//	struct Attachment {
//		void operator=(RenderTarget& _target)
//		{ target = &_target; }
//
//		std::optional<RenderTarget*> target;
//	};
//
//public:
//	std::vector<Attachment> color;
//	Attachment              depth;
//	Attachment              stencil;
//};


class RenderBuffer
	: public RenderTarget
{

};

class TextureUnit {
private:
	using opt_texture_ref_t = std::optional<std::reference_wrapper<Texture>>;

	struct SlotRef {
	public:
		SlotRef(TextureUnit& unit, int slot);

		// @brief Bind texture to slot
		void operator=(Texture& texture);

	private:
		int          m_slot;
		TextureUnit& m_unit;
	};

public:
	TextureUnit();

	SlotRef operator[](int slot);

	// @brief Get the slot where the texture is bound
	std::optional<int> getSlot(Texture& texture);

	// @brief Get index of first empty slot
	std::optional<int> emptySlot() const;

	// @brief Clear slots
	void clear();

	// @brief Max number of textures in a unit allowed by the hardware
	size_t size() const;

	void makeActive();

private:
	std::vector<opt_texture_ref_t> m_textures;
	int                            m_slotCount;
};

}
