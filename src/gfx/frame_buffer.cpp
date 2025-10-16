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
	callPropertySetters(true);
	
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

template <>
void details::common::setProperty(dk::gfx::FrameBuffer& container, const dk::gfx::properties::multisampling& ms)
{
	if (ms == dk::gfx::properties::multisampling::enabled)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);
}
