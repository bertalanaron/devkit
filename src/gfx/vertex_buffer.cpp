#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>

#include <GL/glew.h>

void dk::gfx::VertexBuffer::init()
{
    if (m_vao)
        return;

    glGenBuffers(1, &m_vbo); 

    GLint currentVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    if (0 != currentVAO)
        m_vao = currentVAO;
    else
        glGenVertexArrays(1, &m_vao);
}

void dk::gfx::VertexBuffer::update()
{
    if (m_changedMax <= m_changedMin)
        return;

    int updateBeg = m_changedMin * m_vertexAttributes->size();
    int updateSize = (m_changedMax - m_changedMin + 1) * m_vertexAttributes->size();

    const void* dataBeg = &(m_vertices.data())[updateBeg];

    // Update vertices inide range
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, updateBeg, updateSize, dataBeg);

    m_changedMin = std::numeric_limits<int>::max();
    m_changedMax = std::numeric_limits<int>::min();
}

dk::gfx::VertexBuffer dk::gfx::VertexBuffer::create(VertexFlags flags)
{
    const auto* attributes = VertexAttributes::get(flags);
    return VertexBuffer(attributes, std::move(common::TypelessBuffer(attributes->size())));
}

dk::gfx::VertexBuffer dk::gfx::VertexBuffer::create(const VertexAttributes* vertexAttributes)
{
    return VertexBuffer(vertexAttributes, std::move(common::TypelessBuffer(vertexAttributes->size())));
}

void dk::gfx::VertexBuffer::makeActive()
{
    init(); 

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    if (m_resized) {
        m_resized    = false;
        m_changedMin = std::numeric_limits<int>::max();
        m_changedMax = std::numeric_limits<int>::min();

        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * m_vertices.elem_size(), m_vertices.data(), GL_STATIC_DRAW);
    }
    else
        update();
}

void dk::gfx::VertexBuffer::clear()
{
    m_vertices.clear();
}

void dk::gfx::VertexBuffer::push_back(const std::vector<uint8_t>& vertex)
{
    m_vertices.push_back(vertex.data());
    m_resized = true;
}
