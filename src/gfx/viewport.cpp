#include <devkit/gfx/viewport.h>

#include <gl/GL.h>

void dk::gfx::Viewport::makeActive() const
{
	//spdlog::trace("[gfx] glViewport({},{},{},{})", 
	//	m_offset.x, m_offset.y, m_size.x, m_size.y);
	glViewport(m_offset.x, m_offset.y, m_size.x, m_size.y);
}

dk::gfx::Viewport::Viewport(const glm::ivec2& size)
	: m_size(size)
	, m_offset(0, 0)
{ }

dk::gfx::Viewport::Viewport(const glm::ivec2& size, const glm::ivec2& offset, int offsetTop)
	: m_size(size)
	, m_offset(offset)
	, m_offsetTop(offsetTop)
{ }

bool dk::gfx::Viewport::wrapPoint(glm::ivec2& point, int border) const
{
	const auto offset = m_offset + glm::ivec2(border, border);
	const auto size   = m_size - 2 * glm::ivec2(border, border);
	glm::ivec2 remainder((point - offset) % size);
	if (remainder.x < 0)
		remainder.x += size.x;
	if (remainder.y < 0)
		remainder.y += size.y;
	auto tmp = offset + remainder;
	std::swap(tmp, point);
	return tmp != point;
}

glm::ivec2 dk::gfx::Viewport::nearEdges(const glm::ivec2& point) const
{
	DK_ASSERT((false, "implement this"));
	return glm::ivec2();
}

glm::vec2 dk::gfx::Viewport::normalize(const glm::ivec2& pixels) const
{
	return glm::vec2((float)(pixels.x - m_offset.x) / (float)m_size.x,
		             1 - (float)(pixels.y - m_offsetTop) / (float)m_size.y);
}

double dk::gfx::Viewport::aspectRatio() const
{
	return (double)m_size.x / (double)m_size.y;
}
