#pragma once
#include <devkit/common/utils.h>

namespace dk::gfx::api {

template <typename D>
class Resource {
public:
	const unsigned& handle()
	{
		if (!initialized())
			m_handle = static_cast<D*>(this)->initialize();
		return m_handle;
	}

	bool initialized() const { return m_handle != 0; }

private:
	unsigned m_handle = 0;
};

class VertexBufferArray
	: public Resource<VertexBufferArray>
{
public:
	void bind();

	~VertexBufferArray();

private:
	unsigned initialize();

	inline static size_t s_instanceCounter = 0;

	friend class Resource<VertexBufferArray>;
};

class VertexBufferObject
	: public Resource<VertexBufferObject>
{
public:
	void bind();

private:
	VertexBufferArray m_vao;

	unsigned initialize();

	friend class Resource<VertexBufferObject>;
};

} // namespace dk::gfx
