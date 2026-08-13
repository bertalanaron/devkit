#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>

#include <glad/glad.h>

void dk::gfx::VertexBuffer::makeActive()
{
    // Bind api handle
    m_apiHandle.bind();

    // Upload data to gpu if buffer is dirty
    if (m_buffer.dirty())
    {
        m_buffer.reset_dirty_flag();
        size_t bufferSize = get().size() * get().elem_size();
        glBufferData(GL_ARRAY_BUFFER, bufferSize, get().data(), GL_STATIC_DRAW);
    }
}

dk::gfx::VertexBuffer::operator dk::gfx::Shader::LayoutElement()
{
    Shader::LayoutElement layout;
    layout.attributes = vertexAttributes();
    layout.attributesMask = ~0u;
    layout.divisor = 0;
    layout.bind = [&]() { makeActive(); };
    return layout;
}
