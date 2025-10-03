#pragma once
#include <devkit/gfx/vertex.h>

namespace dk::gfx {

template <typename Vertex>
	requires (is_vertex_v<Vertex>)
struct DrawData {
	Primitive           type;
	std::vector<Vertex> vertices;

	DrawData(Primitive _type, const std::vector<Vertex>& _vertices)
		: type(_type)
		, vertices(_vertices)
	{ }

	DrawData(Primitive _type, std::vector<Vertex>&& _vertices)
		: type(_type)
		, vertices(std::move(_vertices))
	{ }

	DrawData& operator<<(const DrawData& dd)
	{
		if (type != dd.type)
			throw std::runtime_error("DrawData type mismatch");

		vertices.insert(vertices.end(), dd.vertices.cbegin(), dd.vertices.cend());
	}
};

template <typename T>
struct is_draw_data : std::false_type {};

template <typename Vertex>
	requires (is_vertex_v<Vertex>)
struct is_draw_data<DrawData<Vertex>> : std::true_type {};

template <typename T>
constexpr bool is_draw_data_v = is_draw_data<T>::value;

using RGBDrawData  = DrawData<RGBVertex>;
using RGBADrawData = DrawData<RGBAVertex>;

} // dk::gfx
