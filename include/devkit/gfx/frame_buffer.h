#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/gfx/mesh.h>
#include <devkit/gfx/texture.h>
#include <devkit/gfx/viewport.h>

namespace dk::gfx {

class FrameBuffer 
{
private:
	using opt_texture_ref_t = std::optional<std::reference_wrapper<Texture>>;

	struct backbuffer_t {};

public:
	enum class DepthTest   { Disabled, Enabled };
	enum class DepthFunc   { Less, Never, Equal, Lequal, Greater, NotEqual, Gequal, Always };
	enum class Blend       { Disabled, Enabled };
	enum class SrcBlendFactor { One, Zero, SrcAlpha, OneMinusSrcAlpha };
	enum class DstBlendFactor { Zero, One, SrcAlpha, OneMinusSrcAlpha };
	enum class CullFace    { Disabled, Enabled };
	enum class ScissorTest { Disabled, Enabled };
	enum class Multisample { Disabled, Enabled };
	using      LineWidth   = common::UniqueProperty<float, "LineWidth">;
	using      PointSize   = common::UniqueProperty<float, "PointSize">;

	class Config
		: private common::ConfigurationBase<
			DepthTest, DepthFunc, Blend, SrcBlendFactor, DstBlendFactor, 
		    CullFace, ScissorTest, Multisample, LineWidth, PointSize>
	{
	private:
		using Base = common::ConfigurationBase<
			DepthTest, DepthFunc, Blend, SrcBlendFactor, DstBlendFactor, 
			CullFace, ScissorTest, Multisample, LineWidth, PointSize>;

	public:
		using ConfigurationBase::operator();
		using ConfigurationBase::set;
		using ConfigurationBase::get;

		inline friend void to_json(nlohmann::json& j, const Config& config)
		{ to_json(j, (const Base&)config); }

		inline friend void from_json(const nlohmann::json& j, Config& config)
		{ from_json(j, (Base&)config); }

	private:
		using ConfigurationBase::ConfigurationBase;

		friend class FrameBuffer;
	};

	Config config;

public:
	enum class ClearMask { Color = 0x00004000, Depth = 0x00000100 };
	inline friend ClearMask operator|(ClearMask a, ClearMask b) { return ClearMask((int)a | (int)b); }

	FrameBuffer();
	FrameBuffer(FrameBuffer&&) = default;

	// @brief When attaching to 0, viewport is automatically set to texture size
	void attachColor(AttachmentBase& target, int attachmentIndex = 0);

	opt_texture_ref_t color(int attachmentIndex = 0);

	// @brief Create a renderbuffer to hold depth information
	void attachDepth(AttachmentBase& target);

	opt_texture_ref_t depth();

	// @brief Set size and offset of viewport. 
	// When not set, viewport size is the size of the first attachment. 
	void setViewport(const gfx::Viewport& viewport);

	float aspectRatio() const;
	
	// Has to be run on the render thread.
	void clear(ClearMask mask, const glm::vec4& color = dk::colors::black);

	// @brief Draw data bound in the shader using it's layout(...) method. Use a vertex buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count = 1);

	// @brief Draw data bound in the shader using it's layout(...) method. Use an element buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count = 1);

private:
	unsigned m_handle = 0;

	int                          m_maxColorAttachments;
	std::vector<AttachmentBase*> m_colorAttachments;
	AttachmentBase*              m_depthAttachment;

	std::optional<Viewport>      m_viewport;

	inline static std::unordered_map<const void*, Config> s_currentContextConfig{};

	FrameBuffer(backbuffer_t);

	void initializeOrUpdate();

	void makeActive();

	friend FrameBuffer& backBuffer();

	template<typename P> 
	friend void setFrameBufferProperty(FrameBuffer&, const P&);
};

FrameBuffer& backBuffer();

}
