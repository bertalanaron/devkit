#include <devkit/gfx/frame_buffer.h>
#include "context.h"

#include <GL/glew.h>

dk::gfx::FrameBuffer::FrameBuffer()
{ 
	// Get maximum number of color attachments
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_maxColorAttachments);
		
	m_colorAttachments.resize(m_maxColorAttachments);
}

//dk::gfx::FrameBuffer::FrameBuffer(int width, int height, int channels, bool depth)
//	: m_size(width, height)
//{
//	// Get maximum number of color attachments
//	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_maxColorAttachments);
//	
//	m_colorAttachments = std::vector<FrameBuffer::color_attachment_t>(m_maxColorAttachments);
//	m_colorAttachments.at(0) = std::make_unique<Texture>(std::move(Texture::create(width, height, channels)));
//	if (depth)
//		m_depthAttachment = std::make_unique<RenderBuffer>();
//}

//void dk::gfx::FrameBuffer::attachColor(Texture& texture, int attachmentIndex)
//{
//	if (attachmentIndex < 0 || attachmentIndex >= m_maxColorAttachments)
//	{
//		spdlog::error("Trying to attach to incorrect index: {}", attachmentIndex);
//		return;
//	}
//	m_colorAttachments.at(attachmentIndex) = &texture;
//}
//
//void dk::gfx::FrameBuffer::attachColor(RenderBuffer& renderBuffer, int attachmentIndex)
//{
//	if (attachmentIndex < 0 || attachmentIndex >= m_maxColorAttachments)
//	{
//		spdlog::error("Trying to attach to incorrect index: {}", attachmentIndex);
//		return;
//	}
//	m_colorAttachments.at(attachmentIndex) = &renderBuffer;
//}
//
//void dk::gfx::FrameBuffer::detachColor(int attachmentIndex)
//{
//	if (attachmentIndex < 0 || attachmentIndex >= m_maxColorAttachments)
//	{
//		spdlog::error("Trying to detach at incorrect index: {}", attachmentIndex);
//		return;
//	}
//	m_colorAttachments.at(attachmentIndex) = std::monostate{};
//}

void dk::gfx::FrameBuffer::attachColor(AttachmentBase& target, int attachmentIndex)
{
	if (attachmentIndex < 0 || attachmentIndex >= m_maxColorAttachments)
	{
		spdlog::error("Trying to attach to incorrect index: {}", attachmentIndex);
		return;
	}
	m_colorAttachments.at(attachmentIndex) = &target;
}

dk::gfx::FrameBuffer::opt_texture_ref_t dk::gfx::FrameBuffer::color(int attachmentIndex)
{
	auto ptr = m_colorAttachments[attachmentIndex];
	if (!ptr)
		return std::nullopt;

	auto castPtr = dynamic_cast<Texture*>(ptr);
	if (!castPtr)
		return std::nullopt;

	return *castPtr;
}

void dk::gfx::FrameBuffer::attachDepth(AttachmentBase& target)
{
	m_depthAttachment = &target;
}

void dk::gfx::FrameBuffer::clear(ClearMask mask, const glm::vec4& color)
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

#include <GL/glu.h>

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
	return m_viewport.value_or(Viewport(m_colorAttachments[0]->size())).aspectRatio();
}

void dk::gfx::FrameBuffer::initializeOrUpdate()
{
	if (m_handle)
		return;
	glGenFramebuffers(1, &m_handle);
}

void dk::gfx::FrameBuffer::makeActive()
{
	initializeOrUpdate();
	glBindFramebuffer(GL_FRAMEBUFFER, (m_handle == -1) ? 0 : m_handle);

	const Viewport viewport = m_viewport.value_or(Viewport(m_colorAttachments[0]->size()));
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
	for (int i = 0; i < m_colorAttachments.size(); ++i) {
		if (!m_colorAttachments[i])
			continue;

		auto& color         = m_colorAttachments.at(i);
		unsigned attachment = GL_COLOR_ATTACHMENT0 + i;

		color->attachAs(*this, attachment);
		activeColorAttachmentIndices.push_back(i);
	}
	//if (m_colorAttachments.size() > 0)
	//	glDrawBuffers(activeColorAttachmentIndices.size(), activeColorAttachmentIndices.data());

	// Set depth attachment
	if (m_depthAttachment)
		m_depthAttachment->attachAs(*this, GL_DEPTH_ATTACHMENT);

	// TODO:
}

dk::gfx::FrameBuffer::FrameBuffer(backbuffer_t)
	: m_handle(-1)
	, m_maxColorAttachments(1)
	, m_colorAttachments(m_maxColorAttachments)
{
	attachColor(*(new NullAttachment()), 0);
	attachDepth(*(new NullAttachment()));
}

dk::gfx::FrameBuffer& dk::gfx::backBuffer() 
{
	static std::mutex s_backbufferMut;
	static std::unordered_map<const io::WindowContext*, std::unique_ptr<FrameBuffer>> s_frameBuffers;

	std::lock_guard lock(s_backbufferMut);
	auto& fb_ptr = s_frameBuffers[io::GlobalState::currentWindowContext()];
	if (!fb_ptr)
		fb_ptr.reset(new FrameBuffer(FrameBuffer::backbuffer_t{}));
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
