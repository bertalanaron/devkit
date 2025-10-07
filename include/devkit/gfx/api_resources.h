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

unsigned toUnderlying(Channels channels)
{

}

unsigned internalFormat(Channels channels)
{

}

enum class Attachment { Color0, Depth, Stencil, DepthAndStencil };
unsigned toUnderlying(Attachment attachment);

enum class TextureType { Unset, Texture1D, Texture2D, Texture3D, Cubemap, MultisampledTexture2D };
unsigned toUnderlying(TextureType type);
bool canAttachToFrameBuffer(TextureType type);

class Texture
	: public Resource<Texture>
{
public:
	void bind(TextureType textureType);

private:
	unsigned initialize();

	friend class Resource<Texture>;
};

} // namespace dk::gfx::api
