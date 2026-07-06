#include <devkit/gfx/element_buffer.h>

#include <glad/glad.h>

void dk::gfx::ElementBuffer::clear()
{
    m_indices.clear();
    m_resized = true;
}

void dk::gfx::ElementBuffer::push(const index_t& index)
{
    m_indices.push_back(index);
    m_resized = true;
}

void dk::gfx::ElementBuffer::push(const face_t& face)
{
    m_indices.push_back(face[0]);
    m_indices.push_back(face[1]);
    m_indices.push_back(face[2]);
    m_resized = true;
}

void dk::gfx::ElementBuffer::init()
{

    glGenBuffers(1, &m_ebo);

    GLint currentVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    if (0 != currentVAO)
        m_vao = currentVAO;
    else
        glGenVertexArrays(1, &m_vao);
}

void dk::gfx::ElementBuffer::makeActive()
{
    if (m_ebo == 0)
        init();

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    if (m_resized) {
        m_resized    = false;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_t) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);
    }
}

unsigned dk::gfx::ElementBuffer::count()
{
    return m_indices.size();
}
