#include <devkit/gfx/vertex.h>

#include <GL/glew.h>

dk::gfx::VertexFlags dk::gfx::operator|(VertexFlags lhs, VertexFlags rhs)
{
    return VertexFlags((unsigned)lhs | (unsigned)rhs);
}

const dk::gfx::VertexAttributes* dk::gfx::VertexAttributes::get(std::vector<details::gfx::GLType>&& types)
{
    auto it = s_cache.find(types);
    if (it == s_cache.end())
        it = s_cache.insert({ std::vector<details::gfx::GLType>(types), VertexAttributes(std::move(types)) }).first;

    return &it->second;
}

#define TRY_PUSH_VERTEX_ATTRIB_TYPE(name, type) if ((unsigned)flags & (unsigned)dk::gfx::VertexFlags::name) types.push_back(details::gfx::GLType::get<type>())

std::vector<details::gfx::GLType> toGLTypesVector(dk::gfx::VertexFlags flags)
{
    std::vector<details::gfx::GLType> types;
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Position, glm::vec3);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color0  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color1  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color2  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color3  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color4  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color5  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color6  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Color7  , glm::vec4);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Normals , glm::vec3);
    TRY_PUSH_VERTEX_ATTRIB_TYPE(TexCoord, glm::vec2);
    // TODO: I'm not sure about bones
    TRY_PUSH_VERTEX_ATTRIB_TYPE(Bones   , glm::vec4);
    return types;
}

const dk::gfx::VertexAttributes* dk::gfx::VertexAttributes::get(VertexFlags flags)
{
    return get(std::move(toGLTypesVector(flags)));
}

size_t dk::gfx::VertexAttributes::size() const
{
    return m_size;
}

unsigned dk::gfx::VertexAttributes::makePointersActive(size_t indexOffset) const
{
    std::size_t size = 0;
    for (const auto& type : m_types)
        size += type.size * type.repeate;

    std::size_t sizeSoFar = 0;
    GLuint index = indexOffset;
    for (const auto& type : m_types) {
        for (int i = 0; i < type.repeate; ++i) {
            // Enable attrib pointer
            glVertexAttribPointer(index, type.count, type.type, GL_FALSE, size, (void*)sizeSoFar);
            glEnableVertexAttribArray(index);

            // Move pointer
            sizeSoFar += type.size;
            ++index;
        }
    }

    return index;
}

void dk::gfx::VertexAttributes::setPointerDivisors(unsigned divisor, size_t indexOffset) const
{
    GLuint index = indexOffset;
    for (const auto& type : m_types) {
        for (int i = 0; i < type.repeate; ++i) {
            glVertexAttribDivisor(index, divisor);
            ++index;
        }
    }
}

dk::gfx::VertexAttributes::VertexAttributes(std::vector<details::gfx::GLType>&& types)
    : m_types(std::move(types))
    , m_size(std::accumulate(m_types.begin(), m_types.end(), 0, [](const auto& a, const auto& t) { return a + (size_t)t.size * t.repeate; }))
{ }
