#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/gfx/draw_data.h>
#include <devkit/common/runtime_buffer.h>

namespace dk::gfx {

class VertexBuffer {
public:
	template <typename Vertex>
	static VertexBuffer create() 
	{
		return VertexBuffer(Vertex::attributes(), std::move(common::TypelessBuffer(common::id_t<Vertex>{})));
	}

	static VertexBuffer create(VertexFlags flags);

	static VertexBuffer create(const VertexAttributes* vertexAttributes);

	void makeActive();

	void clear();

	template <typename Vertex>
	const Vertex& get(size_t index) const
	{
		DK_ASSERT(("Vertex layout mismatch", Vertex::attributes() == m_vertexAttributes));
		return m_vertices.at<Vertex>(index);
	}

	template <typename Vertex>
	void set(int index, const Vertex& vertex)
	{
		DK_ASSERT(("Vertex layout mismatch", Vertex::attributes() == m_vertexAttributes));
		m_vertices.at<Vertex>(index) = vertex;
		if (index < m_changedMin)
			m_changedMin = index;
		if (index > m_changedMax)
			m_changedMax = index;
	}

	template <typename Vertex>
	void push_back(const Vertex& vertex)
	{
		DK_ASSERT(("Vertex layout mismatch", Vertex::attributes() == m_vertexAttributes));
		m_vertices.push_back(vertex);
		m_resized = true;
	}

	void push_back(const std::vector<uint8_t>& vertex);

	template <typename Vertex>
	void insert(std::vector<Vertex>::const_iterator begin, std::vector<Vertex>::const_iterator end)
	{
		DK_ASSERT(("Vertex layout mismatch", Vertex::attributes() == m_vertexAttributes));
		m_vertices.insert(m_vertices.end(), begin, end);
		m_resized = true;
	}

	size_t size() const
	{ return m_vertices.size(); }

	const auto vertexAttributes() const
	{ return m_vertexAttributes; }

	template <typename Vertex>
	VertexBuffer& operator<<(const DrawData<Vertex>& dd)
	{
		push<Vertex>(dd.vertices.cbegin(), dd.vertices.cend());
		return *this;
	}

private:
	const VertexAttributes* m_vertexAttributes;
	common::TypelessBuffer  m_vertices;

	unsigned m_vao = 0;
	unsigned m_vbo = 0;
	bool     m_resized = true;
	int      m_changedMin = std::numeric_limits<int>::max();
	int      m_changedMax = std::numeric_limits<int>::min();

	VertexBuffer(const dk::gfx::VertexAttributes* vertexAttributes, common::TypelessBuffer&& vertices) 
		: m_vertexAttributes(vertexAttributes)
		, m_vertices(std::move(vertices))
	{ }

	void init();
	void update();
};

}
