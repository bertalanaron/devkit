#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/gfx/mesh.h>
#include <devkit/gfx/texture.h>

namespace dk::gfx::properties {
enum class multisampling { disabled, enabled };
}

namespace details::gfx {

template <typename D>
using FrameBufferProperties = dk::common::DeferredPropertyCollection<D, 
	dk::gfx::properties::backface_culling,
	dk::gfx::properties::depth_test,
	dk::gfx::properties::depth_func,
	dk::gfx::properties::multisampling>;

void setBackbufferViewport(const glm::ivec2& size); 

}

namespace dk::gfx {

class FrameBuffer 
	: public details::gfx::FrameBufferProperties<FrameBuffer> 
{
private:
	using opt_texture_ref_t = std::optional<std::reference_wrapper<Texture>>;

	struct backbuffer_t {};

public:
	enum class ClearMask { Color = 0x00004000, Depth = 0x00000100 };
	inline friend ClearMask operator|(ClearMask a, ClearMask b) { return ClearMask((int)a | (int)b); }

	FrameBuffer();
	FrameBuffer(FrameBuffer&&) = default;

	void attachColor(AttachmentBase& target, int attachmentIndex = 0);

	opt_texture_ref_t color(int attachmentIndex = 0);

	// @brief Create a renderbuffer to hold depth information
	void attachDepth(AttachmentBase& target);

	opt_texture_ref_t depth();

	// @brief Resizes the color and depth buffers (only resizes depth buffer when colorAttachmentIndex is 0)
	void resize(const glm::ivec2& size, int colorAttachment = 0);

	// Has to be run on the render thread.
	void clear(ClearMask mask, const glm::vec4& color = dk::colors::black);

	// @brief Draw data bound in the shader using it's layout(...) method. Use a vertex buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count = 1);

	// @brief Draw data bound in the shader using it's layout(...) method. Use an element buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count = 1);

	float aspectRatio() const;

	void makeActive();

private:
	unsigned m_handle = 0;

	int                          m_maxColorAttachments;
	std::vector<AttachmentBase*> m_colorAttachments;
	AttachmentBase*              m_depthAttachment;

	FrameBuffer(backbuffer_t);

	void initializeOrUpdate();

	friend FrameBuffer& backBuffer();
	friend void details::gfx::setBackbufferViewport(const glm::ivec2& size); 
};

FrameBuffer& backBuffer();

}
