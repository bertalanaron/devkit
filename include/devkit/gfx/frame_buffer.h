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

enum class Clear { Color = 0x00004000, Depth = 0x00000100 };

inline Clear operator|(Clear a, Clear b) { return Clear((int)a | (int)b); }

enum class Mask { Color = BIT(0), Depth = BIT(1), Stencil = BIT(2) };
inline Mask operator|(Mask a, Mask b) { return Mask((int)a | (int)b); }

class FrameBuffer
{
private:
	class Attachment {
	public:
		template <std::derived_from<RenderTarget> T>
		void operator=(T& renderTarget)
		{ m_data = std::ref(renderTarget); }

		template <std::derived_from<RenderTarget> T>
		void operator=(T&& renderTarget)
		{ 
			std::unique_ptr<RenderTarget> ptr = std::make_unique<T>(std::move(renderTarget));
			m_data.emplace<2>(std::move(ptr)); 
		}

		RenderTarget& get();

		const RenderTarget& get() const;

		template <std::derived_from<RenderTarget> T>
		T& get() { return *dynamic_cast<T*>(&get()); }

		template <std::derived_from<RenderTarget> T>
		const T& get() const { return *dynamic_cast<const T*>(&get()); }

		bool owns() const 
		{ return std::holds_alternative<std::unique_ptr<RenderTarget>>(m_data); }

		bool has_value() const
		{ return !std::holds_alternative<std::monostate>(m_data); }

		glm::ivec3 size() const;

	private:
		using data_t = std::variant<std::monostate, 
			std::reference_wrapper<RenderTarget>, 
			std::unique_ptr<RenderTarget>>;

		data_t m_data;
	};

public:
	enum class DepthTest      { Disabled, Enabled };
	enum class DepthFunc      { Less, Never, Equal, Lequal, Greater, NotEqual, Gequal, Always };
	enum class Blend          { Disabled, Enabled };
	enum class SrcBlendFactor { One, Zero, SrcAlpha, OneMinusSrcAlpha };
	enum class DstBlendFactor { Zero, One, SrcAlpha, OneMinusSrcAlpha };
	enum class CullFace       { Disabled, Enabled };
	enum class ScissorTest    { Disabled, Enabled };
	enum class Multisample    { Disabled, Enabled };
	enum class SampleShading  { Disabled, Enabled };
	using      LineWidth      = common::UniqueProperty<float, "LineWidth">;
	using      PointSize      = common::UniqueProperty<float, "PointSize">;

	class Config : DK_CONFIG_SPECIALIZATION(FrameBuffer,
		DepthTest, DepthFunc, Blend, SrcBlendFactor, DstBlendFactor, CullFace,
		ScissorTest, Multisample, SampleShading, LineWidth, PointSize);

	Config config;

	struct DemoScene { };

public:
	FrameBuffer();
	FrameBuffer(api::FrameBuffer::backbuffer_t);
	FrameBuffer(FrameBuffer&&) = default;

	std::vector<Attachment> color;
	Attachment              depth;
	Attachment              stencil;

	// @brief Set size and offset of viewport. 
	// When not set, viewport size is the size of the first attachment. 
	void setViewport(const gfx::Viewport& viewport);

	float aspectRatio() const;
	
	// Has to be run on the render thread.
	void clear(Clear mask, const glm::vec4& color = dk::colors::black);

	void blit(FrameBuffer& input, Mask mask = Mask::Color, int inputColorIndex = 0, int outputColorIndex = 0, 
		      Texture::MagFilter filter = Texture::MagFilter::Linear);

	// @brief Draw data bound in the shader using it's layout(...) method. Use a vertex buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count = 1);

	// @brief Draw data bound in the shader using it's layout(...) method. Use an element buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count = 1);

	void render(DemoScene);

private:
	api::FrameBuffer        m_apiHandle;
	std::optional<Viewport> m_viewport;

	inline static std::unordered_map<const void*, Config> s_currentContextConfig{};

	void makeActive();

	template <std::invocable<RenderBuffer&> F>
	void for_each_target(F&& callback)
	{
		for (int i = 0; i < color.size(); ++i)
			if (color[i].has_value())
				callback(color[i].get());
		if (depth.has_value())
			callback(depth.get());
		if (stencil.has_value())
			callback(stencil.get());
	}

	friend FrameBuffer& backBuffer();

	template<typename P>
	friend void setFrameBufferProperty(FrameBuffer&, const P&);
};

FrameBuffer& backBuffer();

}
