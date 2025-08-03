#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>

namespace dk::gfx {

class Mesh {
public:
	template <typename Vertex>
	static Mesh create()
	{
		return Mesh(std::move(VertexBuffer::create<Vertex>()), std::move(ElementBuffer()));
	}

	static Mesh create(VertexFlags flags);

	auto& vertices() { return m_vertexBuffer; }
	const auto& vertices() const { return m_vertexBuffer; }

	auto& indices() { return m_elementBuffer; }
	const auto& indices() const { return m_elementBuffer; }

	template <typename Vertex>
	void push_back(const Vertex& vertex)
	{
		const auto index = m_vertexBuffer.size();
		m_vertexBuffer.push_back<Vertex>(vertex);
		m_elementBuffer.push(index);
	}

private:
	VertexBuffer  m_vertexBuffer;
	ElementBuffer m_elementBuffer;
	
	Mesh(VertexBuffer&& vb, ElementBuffer&& eb);
};

}
