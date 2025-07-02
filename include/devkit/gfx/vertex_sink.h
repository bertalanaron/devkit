#pragma once
#include <devkit/gfx/common.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/frame_buffer.h>

namespace dk::gfx {

// TODO: use single vertex buffer for all threads
class VertexSink {
public:
	template <typename Vertex>
	static VertexSink create()
	{
		return VertexSink(Vertex::attributes());
	}

	static VertexSink create(VertexFlags flags);

	template <typename Vertex>
	VertexSink& operator<<(const DrawData<Vertex>& drawData)
	{
		m_demux.at((unsigned)drawData.type)->local().insert<Vertex>(drawData.vertices.cbegin(), drawData.vertices.cend());
		return *this;
	}

	template <typename Vertex>
	VertexSink& operator<<(const std::vector<DrawData<Vertex>>& drawData)
	{
		for (const auto& dd : drawData) 
			m_demux.at((unsigned)dd.type)->local().insert<Vertex>(dd.vertices.cbegin(), dd.vertices.cend());
		return *this;
	}

	void clear();

	void draw(Shader& shader, FrameBuffer& frameBuffer);

	void flush(Shader& shader, FrameBuffer& frameBuffer);

private:
	const VertexAttributes* m_vertexAttributes;

	static constexpr size_t s_primitiveCount = magic_enum::enum_count<Primitive>();
	using primitive_demux_t = std::array<std::unique_ptr<common::ThreadDemuxContainer<VertexBuffer>>, s_primitiveCount>;

	primitive_demux_t m_demux;

	static VertexBuffer createVertexBuffer(const VertexAttributes* attributes);

	VertexSink(const VertexAttributes* vertexAttributes);
};

}
