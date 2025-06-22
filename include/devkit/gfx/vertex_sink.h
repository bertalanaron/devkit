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
		getBuffer<Vertex>(drawData) << drawData;
		return *this;
	}

	void clear();

	void draw(Shader& shader, FrameBuffer& frameBuffer);

	void flush(Shader& shader, FrameBuffer& frameBuffer);

private:
	const VertexAttributes* m_vertexAttributes;

	static constexpr size_t s_primitiveCount = magic_enum::enum_count<Primitive>();
	using primitive_map_t = std::array<std::unique_ptr<VertexBuffer>, s_primitiveCount>;
	using thread_map_t    = std::unordered_map<std::thread::id, primitive_map_t>;

	thread_map_t    m_storages;
	std::thread::id m_creatorThread;

	template <typename Vertex>
	VertexBuffer& getBuffer(const DrawData<Vertex>& drawData)
	{
		auto threadId = std::this_thread::get_id();
		auto storageIt = m_storages.find(threadId);
		if (storageIt == m_storages.end())
			storageIt = createStorage(drawData);

		if (!storageIt->second.at((size_t)drawData.type))
			storageIt->second.at((size_t)drawData.type) = std::make_unique<VertexBuffer>(std::move(VertexBuffer::create(m_vertexAttributes)));
		return *storageIt->second.at((size_t)drawData.type);
	}

	template <typename Vertex>
	auto createStorage(const DrawData<Vertex>& drawData) 
	{
		return m_storages.insert({ std::this_thread::get_id(), {} }).first;
	}

	VertexSink(const VertexAttributes* vertexAttributes);
};

}
