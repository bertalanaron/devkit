#include <devkit/gfx/attachment_base.h>

glm::ivec2 dk::gfx::AttachmentBase::size() const 
{
	return m_size;
}

void dk::gfx::AttachmentBase::resize(const glm::ivec2& size)
{
	m_size    = size;
	m_resized = true;
}

float dk::gfx::AttachmentBase::aspectRatio() const
{
	return (float)m_size.x / (float)m_size.y;
}

dk::gfx::AttachmentBase::AttachmentBase()
	: m_size(0, 0)
	, m_resized(true)
{
}

dk::gfx::AttachmentBase::AttachmentBase(const glm::ivec2& size)
	: m_size(size)
	, m_resized(true)
{ }

dk::gfx::NullAttachment::NullAttachment()
	: AttachmentBase()
{ }
