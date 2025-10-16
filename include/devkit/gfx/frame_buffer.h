#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/gfx/mesh.h>
#include <devkit/gfx/texture.h>
#include <devkit/gfx/viewport.h>

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

}

namespace dk::gfx {

enum class Clear { Color = 0x00004000, Depth = 0x00000100 };

inline Clear operator|(Clear a, Clear b) { return Clear((int)a | (int)b); }

class FrameBuffer 
	: public details::gfx::FrameBufferProperties<FrameBuffer> 
{
private:
	struct Attachment {
		void operator=(RenderTarget& _target)
		{ target = &_target; }

		glm::ivec3 size() const 
		{
			return (!target.has_value())
				? glm::ivec3(0, 0, 0) 
				: target.value()->targetSize();  
		}

		std::optional<RenderTarget*> target;
	};

public:
	FrameBuffer();
	FrameBuffer(api::FrameBuffer::backbuffer_t);
	FrameBuffer(FrameBuffer&&) = default;

	std::vector<Attachment> color;
	Attachment              depth;
	Attachment              stencil;

	// @brief Set size and offset of viewport. 
	// When not set, viewport size is the size of the first attachment. 
	void setViewport(const gfx::Viewport& viewport);

	float aspectRatio() const;
	
	// Has to be run on the render thread.
	void clear(Clear mask, const glm::vec4& color = dk::colors::black);

	// @brief Draw data bound in the shader using it's layout(...) method. Use a vertex buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count = 1);

	// @brief Draw data bound in the shader using it's layout(...) method. Use an element buffer for indexing. 
	// Has to be run on the render thread.
	void render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count = 1);

private:
	api::FrameBuffer        m_apiHandle; 
	std::optional<Viewport> m_viewport;

	void makeActive();

	friend FrameBuffer& backBuffer();
};

FrameBuffer& backBuffer();

}
