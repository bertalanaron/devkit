#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/gfx/draw_data.h>
#include <devkit/common/runtime_buffer.h>
#include <devkit/common/typeless.h>
#include <devkit/gfx/api_resources.h>
#include <devkit/gfx/shader.h>

namespace dk::gfx {

class VertexBuffer {
public:
	template <typename Vertex>
	VertexBuffer(common::id_t<Vertex> vertexTypeId)
		: m_attributes(Vertex::attributes())
		, m_buffer(vertexTypeId)
	{ }

	VertexBuffer(VertexFlags flags)
		: m_attributes(VertexAttributes::get(flags))
		, m_buffer(m_attributes->size())
	{ }
	
	auto vertexAttributes() const { return m_attributes; }

	// @brief Get a immutable reference to the underlying buffer
	const auto& get() const { return m_buffer.get(); }

	// @brief Get a mutable reference to the underlying buffer and mark it as dirty
	auto& modify() { return m_buffer.modify(); }

	// @brief Get size of underlying buffer
	size_t size() const
	{ return get().size(); }

	void makeActive();

	template <typename Vertex>
	VertexBuffer& operator<<(const DrawData<Vertex>& dd)
	{
		if (Vertex::attributes() != m_attributes)
			throw std::runtime_error("vertex layout mismatch");
		modify().insert(get().cend(), dd.vertices.begin(), dd.vertices.end());
		return *this;
	}

	operator Shader::LayoutElement();

private:
	using WatchedBuffer = common::watched_object<common::typeless_vector>;

	const VertexAttributes* m_attributes;
	WatchedBuffer           m_buffer;
	api::VertexBufferObject m_apiHandle;
};

}
