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

class FrameBuffer 
	: public details::gfx::FrameBufferProperties<FrameBuffer> 
{
public:
	enum class ClearMask { Color = 0x00004000, Depth = 0x00000100 };

	void clear(ClearMask mask, const glm::vec4& color);

	void render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count = 1);

	void render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count = 1);

	//void draw(Shader& shader, VertexBuffer& vb, Primitive primitive)
	//{
	//	makeActive();
	//	shader.makeActive();
	//	vb.makeActive();
	//	details::gfx::drawArrays(details::gfx::toUnderlying(primitive), 0, vb.size());
	//}

	//void draw(Shader& shader, VertexBuffer& vb, ElementBuffer& eb, Primitive primitive) 
	//{
	//	makeActive();
	//	shader.makeActive();
	//	vb.makeActive();
	//	eb.makeActive();
	//	details::gfx::drawElements(details::gfx::toUnderlying(primitive), GL_UNSIGNED_INT, eb.count(), 0);
	//}

	//void draw(Shader& shader, Mesh& mesh, Primitive primitive) 
	//{
	//	makeActive();
	//	shader.makeActive();
	//	mesh.vertices().makeActive();
	//	mesh.indices().makeActive();
	//	details::gfx::drawElements(details::gfx::toUnderlying(primitive), GL_UNSIGNED_INT, mesh.indices().count(), 0);
	//}

	float aspectRatio() const;

private:
	glm::ivec2 m_size;

	void makeActive();

	friend FrameBuffer& backBuffer();
	friend void details::gfx::setBackbufferViewport(const glm::ivec2& size); 
};

FrameBuffer& backBuffer();

}
