#include <devkit/gfx/api_resources.h>
#include "context.h"

void dk::gfx::api::VertexBufferArray::bind()
{
	auto vao = handle();
	glBindVertexArray(vao);
}

dk::gfx::api::VertexBufferArray::~VertexBufferArray()
{
	if (initialized() && --s_instanceCounter == 0)
		glDeleteVertexArrays(1, &handle());
}

unsigned dk::gfx::api::VertexBufferArray::initialize()
{
	// Get currently bound VAO or 0
	GLuint vao = []{
		GLint curr = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &curr);
		return curr;
	}();
	
	// If no VAO is bound generate one
	if (vao == 0)
		glGenVertexArrays(1, &vao);

	++s_instanceCounter;

	return vao;
}

void dk::gfx::api::VertexBufferObject::bind()
{
	m_vao.bind();
	auto vbo = handle();
	glBindBuffer(GL_ARRAY_BUFFER, handle());
}

unsigned dk::gfx::api::VertexBufferObject::initialize()
{
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	return vbo;
}

unsigned dk::gfx::api::toUnderlying(Attachment attachment)
{
	switch (attachment)
	{
	case Attachment::Color0          : return GL_COLOR_ATTACHMENT0;
	case Attachment::Depth           : return GL_DEPTH_ATTACHMENT;
	case Attachment::Stencil         : return GL_STENCIL_ATTACHMENT;
	case Attachment::DepthAndStencil : return GL_DEPTH_STENCIL_ATTACHMENT;
	default: return 0;
	}
}

unsigned dk::gfx::api::toUnderlying(TextureType type)
{
	switch (type) {
	case TextureType::Texture1D             : return GL_TEXTURE_1D;
	case TextureType::Texture2D             : return GL_TEXTURE_2D;
	case TextureType::Texture3D             : return GL_TEXTURE_3D;
	case TextureType::Cubemap               : return GL_TEXTURE_CUBE_MAP;
	case TextureType::MultisampledTexture2D : return GL_TEXTURE_2D_MULTISAMPLE;
	default: return 0;
	}
}

bool dk::gfx::api::canAttachToFrameBuffer(TextureType type)
{
	if (type == TextureType::Texture2D)             return true;
	if (type == TextureType::MultisampledTexture2D) return true;
	return false;
}

void dk::gfx::api::Texture::bind(TextureType textureType)
{
	auto tex = handle();
	glBindTexture(toUnderlying(textureType), tex);
}

unsigned dk::gfx::api::Texture::initialize()
{
	GLuint tex = 0;
	glGenTextures(1, &tex);
	return tex;
}
