#include <devkit/gfx/frame_buffer.h>
#include "context.h"

#include <GL/glew.h>

dk::gfx::FrameBuffer::FrameBuffer()
	: color(io::GlobalState::hardware().glMaxColorAttachments)
{ }

dk::gfx::FrameBuffer::FrameBuffer(dk::gfx::api::FrameBuffer::backbuffer_t)
	: color(0)
	, m_apiHandle(dk::gfx::api::FrameBuffer::backbuffer_t{})
{ }

void dk::gfx::FrameBuffer::clear(Clear mask, const glm::vec4& color)
{
	makeActive();
	glClearColor(color.r, color.g, color.b, color.a);
	glClear((unsigned)mask);
}

void dk::gfx::FrameBuffer::render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count)
{
	makeActive();
	shader.makeActive();
	(count == 1)
		? glDrawArrays(details::gfx::toUnderlying(primitive), 0, vertexBuffer.size())
		: glDrawArraysInstanced(details::gfx::toUnderlying(primitive), 0, vertexBuffer.size(), count);
}

void dk::gfx::FrameBuffer::render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count)
{
	makeActive();
	shader.makeActive();
	elementBuffer.makeActive();
	(count == 1)
		? glDrawElements(details::gfx::toUnderlying(primitive), elementBuffer.count(), GL_UNSIGNED_INT, 0)
		: glDrawElementsInstanced(details::gfx::toUnderlying(primitive), elementBuffer.count(), GL_UNSIGNED_INT, 0, count);
}

void dk::gfx::FrameBuffer::setViewport(const gfx::Viewport& viewport)
{
	m_viewport = viewport;
}

float dk::gfx::FrameBuffer::aspectRatio() const
{
	return [&]{ return m_viewport.has_value() ? m_viewport.value() : Viewport(color[0].size()); }().aspectRatio();
}

void dk::gfx::FrameBuffer::makeActive()
{
	m_apiHandle.bind();

	const Viewport viewport = [&]{ return m_viewport.has_value() ? m_viewport.value() : Viewport(color[0].size()); }();
	viewport.makeActive();

	// Bind properties to global gl context
	config.for_each([&](const auto& prop) {
		// Skip if value is currently active in the global context
		auto& currentlyBound = s_currentContextConfig[(const void*)dk::io::GlobalState::currentWindowContext()];
		if (prop == currentlyBound.get<std::decay_t<decltype(prop)>>())
			return;
		// Update gl context
		//spdlog::debug("FrameBuffer.{}={}", Config::property_name(prop), std::format("{}", prop));
		setFrameBufferProperty(*this, prop);
		// Update gl context tracker used for skipping
		currentlyBound.set<std::decay_t<decltype(prop)>>(prop);
	});
	
	// Set color attachments
	std::vector<unsigned> activeColorAttachmentIndices;
	for (int i = 0; i < color.size(); ++i) {
		if (!color[i].target.has_value())
			continue;

		color[i].target.value()->setAsTarget(api::Attachment::Color0, i);

		activeColorAttachmentIndices.push_back(i);
	}
	if (color.size() > 0)
		glDrawBuffers(activeColorAttachmentIndices.size(), activeColorAttachmentIndices.data());

	// Set depth attachment
	if (depth.target.has_value())
		depth.target.value()->setAsTarget(api::Attachment::Depth);
}

dk::gfx::FrameBuffer& dk::gfx::backBuffer() 
{
	static std::mutex s_backbufferMut;
	static std::unordered_map<const io::WindowContext*, std::unique_ptr<FrameBuffer>> s_frameBuffers;

	std::lock_guard lock(s_backbufferMut);
	auto& fb_ptr = s_frameBuffers[io::GlobalState::currentWindowContext()];
	if (!fb_ptr)
		fb_ptr.reset(new FrameBuffer(api::FrameBuffer::backbuffer_t{}));
	return *fb_ptr;
}

#define __DK_TABLE_FRAMEBUFFER_TOGGLE_PROPERTY(F, ...)           \
	/*                                  Type | GL Enum        */ \
	F( __VA_ARGS__ __VA_OPT__(,) DepthTest   , GL_DEPTH_TEST   ) \
	F( __VA_ARGS__ __VA_OPT__(,) Blend       , GL_BLEND        ) \
	F( __VA_ARGS__ __VA_OPT__(,) CullFace    , GL_CULL_FACE    ) \
	F( __VA_ARGS__ __VA_OPT__(,) ScissorTest , GL_SCISSOR_TEST ) \
	F( __VA_ARGS__ __VA_OPT__(,) Multisample , GL_MULTISAMPLE  ) \
	/* end of table */
#define __DK_GL_ENABLEDISABLE(type, glEnum)                                                        \
	if (value == std::decay_t<decltype(value)>::Enabled) glEnable(glEnum); else glDisable(glEnum); \
	/* end of macro */
#define __DK_FRAMEBUFFER_DECL_GLTOGGLE_PROPERTYSETTER(type, glEnum)                               \
	template<> void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::type& value) \
	{ __DK_GL_ENABLEDISABLE(type, glEnum); }                                                      \
	/* end of macro */
__DK_TABLE_FRAMEBUFFER_TOGGLE_PROPERTY(__DK_FRAMEBUFFER_DECL_GLTOGGLE_PROPERTYSETTER)

template<>
void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::DepthFunc& depthFunc)
{ 
	auto underlying = [&]{
		switch (depthFunc) {
		case FrameBuffer::DepthFunc::Less    : return GL_LESS;
		case FrameBuffer::DepthFunc::Never   : return GL_NEVER;
		case FrameBuffer::DepthFunc::Equal   : return GL_EQUAL;
		case FrameBuffer::DepthFunc::Lequal  : return GL_LEQUAL;
		case FrameBuffer::DepthFunc::Greater : return GL_GREATER;
		case FrameBuffer::DepthFunc::NotEqual: return GL_NOTEQUAL;
		case FrameBuffer::DepthFunc::Gequal  : return GL_GEQUAL;
		case FrameBuffer::DepthFunc::Always  : return GL_ALWAYS;
		default: return 0;
		}
	}();
	glDepthFunc(underlying);
}

template <typename E>
unsigned blendFactorToUnderlying(const E& value)
{
	switch (value)
	{
	case E::One             : GL_ONE;
	case E::Zero            : GL_ZERO;
	case E::SrcAlpha        : GL_SRC_ALPHA;
	case E::OneMinusSrcAlpha: GL_ONE_MINUS_SRC_ALPHA;
	default: return 0;
	}
}

template<>
void dk::gfx::setFrameBufferProperty(FrameBuffer& frameBuffer, const FrameBuffer::SrcBlendFactor& srcFactor)
{
	glBlendFunc(blendFactorToUnderlying(srcFactor), 
		        blendFactorToUnderlying(frameBuffer.config.get<FrameBuffer::DstBlendFactor>()));
}

template<>
void dk::gfx::setFrameBufferProperty(FrameBuffer& frameBuffer, const FrameBuffer::DstBlendFactor& dstFactor)
{
	glBlendFunc(blendFactorToUnderlying(frameBuffer.config.get<FrameBuffer::SrcBlendFactor>()), 
		        blendFactorToUnderlying(dstFactor));
}

template <>
void dk::gfx::setFrameBufferProperty(FrameBuffer& frameBuffer, const FrameBuffer::SampleShading& sampleShading)
{
	if (!GLEW_ARB_sample_shading)
	{
		static bool logged = false;
		if (!logged)
		{
			logged = true;
			spdlog::warn("[gfx] Sample shading is not available");
		}
		return;
	}

	if (sampleShading == std::decay_t<decltype(sampleShading)>::Enabled) {
		glEnable(GL_SAMPLE_SHADING);
		glMinSampleShading(1.0);
	}
	else 
		glDisable(GL_SAMPLE_SHADING);
}

template <>
void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::LineWidth& lineWidth)
{ glLineWidth(lineWidth.value); }

template <>
void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::PointSize& pointSize)
{ glPointSize(pointSize.value); }
