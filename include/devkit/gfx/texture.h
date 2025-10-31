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

public:
	virtual ~RenderTarget() { }

	friend class FrameBuffer;
};

class Texture {
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

	virtual ~Texture() { }

protected:
	using Initializer = std::optional<std::function<void(unsigned, Texture*)>>;

	api::TextureType m_type;
	api::Texture     m_apiHandle;
	Channels         m_channels;
	Initializer      m_initializer;

	void bindToUnit(unsigned unit);

	void updateOrInitializeAndBind();

	template <typename P>
	friend void setTextureProperty(Texture&, const P&);

	friend class TextureUnit;
};

class Texture1D
	: public Texture
	, public RenderTarget
{
public:
	Texture1D(Texture1D&&)            = default;
	Texture1D& operator=(Texture1D&&) = default;

private:
	int m_size = 0;

	void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

	glm::ivec3 targetSize() const override
	{ return { m_size, 1, 1 }; }
};

class Texture2D
	: public Texture
	, public RenderTarget
{
public:
	Texture2D(Texture2D&&)            = default;
	Texture2D& operator=(Texture2D&&) = default;

	Texture2D()
		: Texture(api::TextureType::Unset, Channels::R)
		, m_size(0, 0)
	{ }

	Texture2D(const glm::ivec2& size, Channels channels = Channels::RGBA);

	Texture2D(const std::filesystem::path& path);

	// @breif Call after graphics context was initalized
	static Texture2D loadFromFileAndInitialize(const std::string& path);

	glm::ivec2 size() const { return m_size; }

	// @brief Resizing the texture requires reallocation so it's data is lost
	void resize(const glm::ivec2& size);

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
};

class MultisampledTexture2D
	: public Texture
	, public RenderTarget
{
public:
	MultisampledTexture2D(MultisampledTexture2D&&)            = default;
	MultisampledTexture2D& operator=(MultisampledTexture2D&&) = default;

	MultisampledTexture2D(const glm::ivec2& size, unsigned samples, Channels channels = Channels::RGBA);

	glm::ivec2 size() const { return m_size; }

	// @brief Resizing the texture requires reallocation so it's data is lost
	void resize(const glm::ivec2& size);

private:
	glm::ivec2 m_size;
	unsigned   m_samples;

	void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

	glm::ivec3 targetSize() const override
	{ return { m_size.x, m_size.y, 1 }; }

	unsigned samples() const override { return m_samples; }
};

class Texture2DArray 
	: public Texture
{
public:
	class Layer 
		: public RenderTarget
	{
	public:
		glm::ivec2 size() const { return m_array->size(); }

		// @brief Resizes each layer of the texture array. 
		// Resizing the texture requires reallocation so it's data is lost
		void resize(const glm::ivec2& size) { m_array->resize(size); }

	private:
		Texture2DArray* m_array;
		int             m_layerIdx;

		Layer(Texture2DArray& array, int layerIdx)
			: m_array(&array)
			, m_layerIdx(layerIdx)
		{ }

		void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

		glm::ivec3 targetSize() const override
		{ return { m_array->m_size.x, m_array->m_size.y, m_array->m_layers }; }

		friend class Texture2DArray;
	};

public:
	// @brief Constructor for lazy initialization, results in invalid state
	Texture2DArray()
		: Texture(api::TextureType::Unset, Channels::R)
		, m_size(0, 0)
		, m_layers(0)
	{ }
	Texture2DArray(Texture2DArray&&)            = default;
	Texture2DArray& operator=(Texture2DArray&&) = default;

	// @brief Returns a reference to a single layer which can be bound to a framebuffer
	Layer operator[](int layer);

	// @brief Create empty texture array
	Texture2DArray(const glm::ivec2& size, int layers, Channels channels = Channels::RGBA);

	// @brief Load multiple files into array. Each file must have the same format and same size. 
	Texture2DArray(const std::vector<std::filesystem::path>& files);

	glm::ivec2 size() const { return m_size; }

	int layers() const { return m_layers; }

	void resize(const glm::ivec2&);

private:
	glm::ivec2 m_size;
	int        m_layers;
};

class RenderBuffer
	: public RenderTarget
{
public:
	RenderBuffer()                          = default;
	RenderBuffer(RenderBuffer&&)            = default;
	RenderBuffer& operator=(RenderBuffer&&) = default;

	RenderBuffer(const glm::ivec2& size, Channels channels = Channels::RGBA, unsigned samples = 1);

	glm::ivec2 size() const { return m_size; }

	void resize(const glm::ivec2& size);

private:
	using Initializer = std::optional<std::function<void(unsigned, RenderBuffer*)>>;
	
	bool              m_valid = false;
	api::RenderBuffer m_apiHandle;
	Initializer       m_initializer;
	Channels          m_channels;
	glm::ivec2        m_size;
	unsigned          m_samples;

	void setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level) override;

	glm::ivec3 targetSize() const override
	{ return { m_size.x, m_size.y, 1 }; }

	void updateOrInitializeAndBind();
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
