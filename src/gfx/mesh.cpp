#include <devkit/gfx/mesh.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

dk::gfx::Mesh dk::gfx::Mesh::create(VertexFlags flags)
{
	return Mesh(std::move(VertexBuffer::create(flags)), std::move(ElementBuffer()));
}

dk::gfx::Mesh::Mesh(VertexBuffer&& vb, ElementBuffer&& eb)
	: m_vertexBuffer(std::move(vb))
	, m_elementBuffer(std::move(eb))
{ }
