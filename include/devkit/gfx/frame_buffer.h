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

class FrameBuffer
{
private:
	struct Attachment {
		void operator=(RenderTarget& _target)
		{ target = &_target; }

		glm::ivec3 size() const
		{
			return (!target.has_value())
				? glm::ivec3(0, 0, 0)
				: target.value()->targetSize();
		}

		std::optional<RenderTarget*> target;
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

	// @brief Draw data bound in the shader using it's layout(...) method. Use a vertex buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count = 1);

	// @brief Draw data bound in the shader using it's layout(...) method. Use an element buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count = 1);

private:
	api::FrameBuffer        m_apiHandle;
	std::optional<Viewport> m_viewport;

	inline static std::unordered_map<const void*, Config> s_currentContextConfig{};

	void makeActive();

	friend FrameBuffer& backBuffer();

	template<typename P>
	friend void setFrameBufferProperty(FrameBuffer&, const P&);
};

FrameBuffer& backBuffer();

}
