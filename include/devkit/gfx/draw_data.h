#pragma once
#include <devkit/gfx/vertex.h>

namespace dk::gfx {

template <typename Vertex>
struct DrawData {
	Primitive           type;
	std::vector<Vertex> vertices;

	constexpr DrawData(Primitive _type, const std::vector<Vertex>& _vertices)
		: type(_type)
		, vertices(_vertices)
	{ }

	constexpr DrawData(Primitive _type, std::vector<Vertex>&& _vertices)
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

using RGBDrawData  = DrawData<RGBVertex>;
using RGBADrawData = DrawData<RGBAVertex>;

} // dk::gfx
