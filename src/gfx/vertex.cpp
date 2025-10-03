#include <devkit/gfx/vertex.h>

#include <GL/glew.h>

const dk::gfx::VertexAttributes* dk::gfx::VertexAttributes::get(std::vector<details::gfx::GLType>&& types)
{
    auto it = s_cache.find(types);
    if (it == s_cache.end())
        it = s_cache.insert({ std::vector<details::gfx::GLType>(types), VertexAttributes(std::move(types)) }).first;

    return &it->second;
}

#define TRY_PUSH_VERTEX_ATTRIB_TYPE(index, name, type) if ((unsigned)flags & (unsigned)dk::gfx::VertexFlags::name) types.push_back(details::gfx::GLType::get<type>());

std::vector<details::gfx::GLType> toGLTypesVector(dk::gfx::VertexFlags flags)
{
    std::vector<details::gfx::GLType> types;
    DK_VERTEXFLAGS_TABLE(TRY_PUSH_VERTEX_ATTRIB_TYPE)
    return types;
}

#define __DK_PUSH_VERTEX_COMPONENT(index, name, type)                            \
	if ((unsigned)vf & (unsigned)VertexFlags::name) {                            \
		result.emplace(VertexFlags::name, std::make_pair(offset, sizeof(type))); \
		offset += sizeof(type);                                                  \
	}                                                                            \
	/* end of macro */

const dk::gfx::VertexAttributes* dk::gfx::VertexAttributes::get(VertexFlags flags)
{
    return get(std::move(toGLTypesVector(flags)));
}

size_t dk::gfx::VertexAttributes::size() const
{
    return m_size;
}

unsigned dk::gfx::VertexAttributes::makePointersActive(size_t indexOffset, unsigned mask) const
{
    std::size_t size = 0;
    for (const auto& type : m_types)
        size += type.size * type.repeate;

    std::size_t sizeSoFar = 0;
    GLuint index = indexOffset;
    for (const auto& type : m_types) {
        for (int i = 0; i < type.repeate; ++i) {
            // Check mask if pointer is active
            const bool isActive = mask & 1u << (index - indexOffset);
            
            // Enable attrib pointer
            if (isActive)
            {
                glVertexAttribPointer(index, type.count, type.type, GL_FALSE, size, (void*)sizeSoFar);
                glEnableVertexAttribArray(index);
            }

            // Move pointer
            sizeSoFar += type.size;
            ++index;
        }
    }

    return index;
}

void dk::gfx::VertexAttributes::setPointerDivisors(unsigned divisor, size_t indexOffset, unsigned mask) const
{
    GLuint index = indexOffset;
    for (const auto& type : m_types) {
        for (int i = 0; i < type.repeate; ++i) {
            const bool isActive = mask & 1u << (index - indexOffset);
            if (isActive);
                glVertexAttribDivisor(index, divisor);
            ++index;
        }
    }
}

dk::gfx::VertexAttributes::VertexAttributes(std::vector<details::gfx::GLType>&& types)
    : m_types(std::move(types))
    , m_size(std::accumulate(m_types.begin(), m_types.end(), 0, [](const auto& a, const auto& t) { return a + (size_t)t.size * t.repeate; }))
{ }
