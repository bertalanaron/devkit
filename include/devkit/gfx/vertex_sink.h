#pragma once
#include <devkit/gfx/common.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/frame_buffer.h>

namespace dk::gfx {

// @brief Threadsafe vertex buffer container for multi threaded drawing. 
class VertexSink {
private:
	static constexpr size_t s_primitiveCount = magic_enum::enum_count<Primitive>();

	using ThreadLocalBuffer  = common::ThreadDemuxContainer<VertexBuffer>;
	using ThreadDemuxStorage = std::array<std::unique_ptr<ThreadLocalBuffer>, s_primitiveCount>;

public:
	template <typename Vertex>
	VertexSink(common::id_t<Vertex> vertTypeId)
		: m_vertexAttributes(Vertex::attributes())
	{ std::ranges::for_each(m_demux, initBuffer<Vertex>); }

	VertexSink(VertexFlags flags)
		: m_vertexAttributes(VertexAttributes::get(flags))
	{ std::ranges::for_each(m_demux, std::bind_back(initBufferF, flags)); }

	template <typename Vertex>
	VertexSink& operator<<(const DrawData<Vertex>& dd)
	{
		auto& buffer = m_demux.at((unsigned)dd.type)->local();
		buffer.modify().insert(buffer.get().cend(), dd.vertices.begin(), dd.vertices.end());
		return *this;
	}

	template <typename Range>
		requires(std::ranges::input_range<Range>)
	VertexSink& operator<<(Range&& dds)
	{
		using value_t = std::ranges::range_value_t<Range>;
		static_assert(is_draw_data_v<value_t>, 
			"value type of range must be gfx::DrawData");

		std::ranges::for_each(dds, [&](const auto& dd) { (*this) << dd; });
		return *this;
	}

	// @param vertices Range of vertices
	template <typename Range>
	VertexSink& insert_back(dk::gfx::Primitive primitive, Range&& vertices)
	{
		using value_t = std::ranges::range_value_t<Range>;
		static_assert(is_vertex_v<value_t>, 
			"value type of range must be gfx::Vertex or derived from gfx::Vertex");

		auto& buffer = m_demux.at((unsigned)primitive)->local();
		buffer.modify().insert(buffer.get().cend(), vertices.cbegin(), vertices.cend());
		return *this;
	}

	void clear();

	void draw(Shader& shader, FrameBuffer& frameBuffer);

	void flush(Shader& shader, FrameBuffer& frameBuffer);

private:
	const VertexAttributes* m_vertexAttributes;
	ThreadDemuxStorage      m_demux;

	template <typename Vertex>
	static void initBuffer(std::unique_ptr<ThreadLocalBuffer>& ptr)
	{ ptr.reset(new ThreadLocalBuffer([] { return VertexBuffer(common::id<Vertex>); })); }

	static void initBufferF(std::unique_ptr<ThreadLocalBuffer>& ptr, VertexFlags flags)
	{ ptr.reset(new ThreadLocalBuffer([flags] { return VertexBuffer(flags); })); }
};

}
