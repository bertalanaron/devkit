#include <devkit/gfx/frame_buffer.h>

#include <GL/glew.h>

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
	// TODO:
}

dk::gfx::FrameBuffer& dk::gfx::backBuffer() 
{
	static FrameBuffer c_backBuffer;
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
