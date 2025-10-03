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
