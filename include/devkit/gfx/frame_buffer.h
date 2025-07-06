#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/gfx/mesh.h>

namespace details::gfx {

template <typename D>
using FrameBufferProperties = dk::common::DeferredPropertyCollection<D, 
	dk::gfx::properties::backface_culling,
	dk::gfx::properties::depth_test,
	dk::gfx::properties::depth_func>;

void setBackbufferViewport(const glm::ivec2& size); 

}

namespace dk::gfx {

class RenderBuffer {

};

class FrameBuffer 
	: public details::gfx::FrameBufferProperties<FrameBuffer> 
{
private:
	using opt_texture_ref_t = std::optional<std::reference_wrapper<Texture>>;

	struct backbuffer_t {};

public:
	enum class ClearMask { Color = 0x00004000, Depth = 0x00000100 };
	inline friend ClearMask operator|(ClearMask a, ClearMask b) { return ClearMask((int)a | (int)b); }

	// @brief Create framebuffer and texture together. 
	// Use this constructor if the texture doesn't need to be reused. 
	FrameBuffer(int width, int height, int channels, bool depth);

	// @brief Create framebuffer without texture. 
	// Attach shared texture later. 
	FrameBuffer();
	FrameBuffer(FrameBuffer&&) = default;

	void attachColor(Texture&, int attachmentIndex = 0);
	void attachColor(RenderBuffer&, int attachmentIndex = 0);
	void detachColor(int attachmentIndex = 0);

	opt_texture_ref_t color(int attachmentIndex = 0);

	// @brief Create a renderbuffer to hold depth information
	void attachDepth();
	void attachDepth(Texture&);
	void attachDepth(RenderBuffer&);
	void detachDepth();

	opt_texture_ref_t depth();

	// @brief Resizes the color and depth buffers (only resizes depth buffer when colorAttachmentIndex is 0)
	void resize(int width, int height, int colorAttachment = 0);

	// Has to be run on the render thread.
	void clear(ClearMask mask, const glm::vec4& color = dk::colors::black);

	// @brief Draw data bound in the shader using it's layout(...) method. Use a vertex buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count = 1);

	// @brief Draw data bound in the shader using it's layout(...) method. Use an element buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count = 1);

	float aspectRatio() const;

private:
	glm::ivec2 m_size;

	using color_attachment_t = std::variant<std::monostate, Texture*, RenderBuffer*, std::unique_ptr<Texture>>;
	using depth_attachment_t = std::variant<std::monostate, Texture*, RenderBuffer*, std::unique_ptr<RenderBuffer>>;

	int                             m_maxColorAttachments;
	std::vector<color_attachment_t> m_colorAttachments;
	depth_attachment_t              m_depthAttachment;

	FrameBuffer(backbuffer_t);

	void init();
	void makeActive();

	friend FrameBuffer& backBuffer();
	friend void details::gfx::setBackbufferViewport(const glm::ivec2& size); 
};

FrameBuffer& backBuffer();

}
