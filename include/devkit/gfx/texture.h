#pragma once
#include <devkit/gfx/common.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/attachment_base.h>

namespace dk::gfx::properties {

enum class min_filter { nearest, linear, linear_mipmap_linear, linear_mipmap_nearest, nearest_mipmap_linear, nearest_mipmap_nearest };
enum class mag_filter { nearest, linear };

}

namespace details::gfx {

template <typename D>
using TextureProperties = dk::common::DeferredPropertyCollection<D, 
	dk::gfx::properties::min_filter,
	dk::gfx::properties::mag_filter>;

int toUnderlying(dk::gfx::properties::min_filter);
int toUnderlying(dk::gfx::properties::mag_filter);

}

namespace dk::gfx {

class FrameBuffer;

class Texture
	: public details::gfx::TextureProperties<Texture>
	, public AttachmentBase
{
public:
	enum class Type { Normal, Cubemap, /* TODO: Multisample */ };

	static Texture load(const std::string& path);

	// @brief left, right, top, bottom front, back,
	static Texture loadCubeMap(const std::array<std::string, 6>& paths);

	static Texture create(unsigned width, unsigned height, std::vector<uint8_t>&& pixels, int channels);

	// @brief Create empty texture
	static Texture create(unsigned width, unsigned height, int channels);

	void makeActive(int unit);

	Type type() const;

	void showAsImGuiImage() const;

	Texture();

private:
	using opt_pixels_t = std::optional<std::variant<std::vector<uint8_t>, std::array<std::vector<uint8_t>, 6>>>;

	Type         m_type = Type::Normal;
	unsigned int m_handle = 0;
	int          m_channels = 0;

	opt_pixels_t m_opt_pixels;

	void initializeOrUpdate() override;

	void attachAs(FrameBuffer& buffer, unsigned underlyingAttachmentIndex) override;

	template <typename D, typename E>
	friend void details::common::setProperty(D&, const E&);
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

namespace details::gfx {

int toUnderlying(const dk::gfx::Texture::Type& type);

}
