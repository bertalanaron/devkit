#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>

namespace dk::gfx {

class Mesh {
public:
	void draw();
	void clear();
	void flush();

	static Mesh load(const std::string& path);

	template <typename Vertex>
	static Mesh create()
	{
		return Mesh(std::move(VertexBuffer::create<Vertex>()), std::move(ElementBuffer()));
	}
	static Mesh create(VertexFlags flags);

	auto& vertices() { return m_vertexBuffer; }
	auto& indices() { return m_elementBuffer; }

private:
	VertexBuffer  m_vertexBuffer;
	ElementBuffer m_elementBuffer;
	
	Mesh(VertexBuffer&& vb, ElementBuffer&& eb)
		: m_vertexBuffer(std::move(vb))
		, m_elementBuffer(std::move(eb))
	{ }
};

//template<typename Vertex>
//void Mesh<Vertex>::draw()
//{
//}

}
