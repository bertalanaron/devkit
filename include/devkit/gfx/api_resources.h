#pragma once
#include <devkit/common/utils.h>

namespace dk::gfx::api {

template <typename D, typename... Args>
class Resource {
public:
	const unsigned& handle(Args... args)
	{
		if (!initialized())
			m_handle = static_cast<D*>(this)->initialize(std::forward<decltype(args)>(args)...);
		return m_handle;
	}

	bool initialized() const { return m_handle != 0; }

protected:
	// @brief Doesn't delete resource, just resets handle to 0
	void resetHandle() { m_handle = 0; }

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

enum class Attachment { Color0, Depth, Stencil, DepthAndStencil };
unsigned toUnderlying(Attachment attachment);

enum class TextureType { Unset, Texture1D, Texture2D, Texture3D, Cubemap, MultisampledTexture2D, Texture2DArray };
unsigned toUnderlying(TextureType type);

class Texture
	: public Resource<Texture>
{
public:
	void bind(TextureType textureType);

private:
	unsigned initialize();

	friend class Resource<Texture>;
};

class RenderBuffer
	: public Resource<RenderBuffer>
{
public:
	void bind();

private:
	unsigned initialize();

	friend class Resource<RenderBuffer>;
};

class FrameBuffer
	: public Resource<FrameBuffer>
{
public:
	struct backbuffer_t { };

	FrameBuffer() = default;
	FrameBuffer(backbuffer_t);

	void bind();

private:
	bool m_isBackbuffer = false;

	unsigned initialize();

	friend class Resource<FrameBuffer>;
};

enum class ShaderType { Unset = -1, Fragment = 0, Vertex, Geometry, TessellationControl, TessellationEvaluation };
unsigned toUnderlying(ShaderType);

void writeShaderCompilationErrorInfo(unsigned program);

class Shader 
	: public Resource<Shader, ShaderType>
{
public:
	void compile(ShaderType type, const std::string& code);

private:
	ShaderType m_type = ShaderType::Unset;

	unsigned initialize(ShaderType type);

	friend class Resource<Shader, ShaderType>;
};

class Program 
	: public Resource<Program>
{
public:
	void bind();

private:
	unsigned initialize();

	friend class Resource<Program>;
};

} // namespace dk::gfx::api
