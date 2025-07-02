#include <devkit/gfx/frame_buffer.h>

#include <GL/glew.h>

dk::gfx::FrameBuffer::FrameBuffer(int width, int height, int channels, bool depth)
	: m_size(width, height)
{
	// Get maximum number of color attachments
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_maxColorAttachments);
	
	m_colorAttachments = std::vector<FrameBuffer::color_attachment_t>(m_maxColorAttachments);
	m_colorAttachments.at(0) = std::make_unique<Texture>(std::move(Texture::create(width, height, channels)));
	if (depth)
		m_depthAttachment = std::make_unique<RenderBuffer>();
}

void dk::gfx::FrameBuffer::attachColor(Texture& texture, int attachmentIndex)
{
	if (attachmentIndex < 0 || attachmentIndex >= m_maxColorAttachments)
	{
		spdlog::error("Trying to attach to incorrect index: {}", attachmentIndex);
		return;
	}
	m_colorAttachments.at(attachmentIndex) = &texture;
}

void dk::gfx::FrameBuffer::attachColor(RenderBuffer& renderBuffer, int attachmentIndex)
{
	if (attachmentIndex < 0 || attachmentIndex >= m_maxColorAttachments)
	{
		spdlog::error("Trying to attach to incorrect index: {}", attachmentIndex);
		return;
	}
	m_colorAttachments.at(attachmentIndex) = &renderBuffer;
}

void dk::gfx::FrameBuffer::detachColor(int attachmentIndex)
{
	if (attachmentIndex < 0 || attachmentIndex >= m_maxColorAttachments)
	{
		spdlog::error("Trying to detach at incorrect index: {}", attachmentIndex);
		return;
	}
	m_colorAttachments.at(attachmentIndex) = std::monostate{};
}

dk::gfx::FrameBuffer::opt_texture_ref_t dk::gfx::FrameBuffer::color(int attachmentIndex)
{
	auto& attachment = m_colorAttachments.at(attachmentIndex);
	if (auto ptr = std::get_if<Texture*>(&attachment)) {
		if (*ptr)
			return std::ref(**ptr);
	} else if (auto uptr = std::get_if<std::unique_ptr<Texture>>(&attachment)) {
		if (*uptr)
			return std::ref(**uptr);
	}
	return std::nullopt;
}

void dk::gfx::FrameBuffer::clear(ClearMask mask, const glm::vec4& color)
{
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

float dk::gfx::FrameBuffer::aspectRatio() const
{
	return (float)m_size.x / (float)m_size.y;
}

void dk::gfx::FrameBuffer::makeActive()
{
	glViewport(0, 0, m_size.x, m_size.y);

	callPropertySetters(true);
	
	// Set color attachments
	std::vector<unsigned> activeColorAttachmentIndices;
	for (int i = 0; i < m_colorAttachments.size(); ++i) {
		auto& color = m_colorAttachments.at(i);
		unsigned attachment = GL_COLOR_ATTACHMENT0 + i;

		// Attach texture
		if (auto ptr = std::get_if<Texture*>(&color)) {
			if (!*ptr)
				continue;
			activeColorAttachmentIndices.push_back(attachment);
			(*ptr)->attachTo(*this, i);

		// Attach owned texture
		} else if (auto uptr = std::get_if<std::unique_ptr<Texture>>(&color)) {
			if (*uptr)
				continue;
			activeColorAttachmentIndices.push_back(attachment);
			(*ptr)->attachTo(*this, i);
		// Attach render buffer
		} else if (auto ptr = std::get_if<RenderBuffer*>(&color)) {
			if (*ptr)
				continue;
			activeColorAttachmentIndices.push_back(attachment);
			// TODO:
		}
	}
	if (m_colorAttachments.size() > 0)
		glDrawBuffers(activeColorAttachmentIndices.size(), activeColorAttachmentIndices.data());

	// TODO:
}

dk::gfx::FrameBuffer::FrameBuffer(backbuffer_t)
	: m_size(0, 0)
	, m_maxColorAttachments(0)
{
	// TODO:
}

dk::gfx::FrameBuffer& dk::gfx::backBuffer() 
{
	static FrameBuffer c_backBuffer(FrameBuffer::backbuffer_t{});
	return c_backBuffer;
}

void details::gfx::setBackbufferViewport(const glm::ivec2& size)
{
	dk::gfx::backBuffer().m_size = size;
}

template <>
void details::common::setProperty(dk::gfx::FrameBuffer& container, const dk::gfx::properties::backface_culling& bc)
{
	if ((bool)bc)
		glEnable(GL_CULL_FACE);
	else 
		glDisable(GL_CULL_FACE);
}

template <>
void details::common::setProperty(dk::gfx::FrameBuffer& container, const dk::gfx::properties::depth_func& df)
{
	glDepthFunc(details::gfx::toUnderlying(df));
}

template <>
void details::common::setProperty(dk::gfx::FrameBuffer& container, const dk::gfx::properties::depth_test& dt)
{
	if (dt == dk::gfx::properties::depth_test::enabled)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}
