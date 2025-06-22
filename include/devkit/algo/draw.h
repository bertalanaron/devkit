#pragma once
#include <devkit/gfx/draw_data.h>
#include <devkit/algo/geometry.h>

namespace dk::gfx {

[[nodiscard]] RGBADrawData draw(const glm::dvec3& point, const glm::vec4& color) 
{
	return RGBADrawData(Primitive::Points, {
		RGBAVertex(point, color)
	});
}

// @brief Draws a 3d edge using a uniform color
[[nodiscard]] RGBADrawData draw(const dk::geom::edge3& edge, const glm::vec4& color) 
{
	return RGBADrawData(Primitive::Lines, {
		RGBAVertex(edge[0], color),
		RGBAVertex(edge[1], color)
	});
}

// @brief Draws the 2d edge on a plane using a uniform color: (0,0) is at the plains defined point.
//
// Example: dk::gfx::draw({glm::dvec2{0,0},{1,1}}, dk::colors::red, dk::geom::plane::Y(), dk::geom::axis::Y)
// @param up - The vector which is used to get the 2d axis with in combination with the plane's normal. (right = plane.normal x up)
[[nodiscard]] RGBADrawData draw(const dk::geom::edge2& edge, const glm::vec4& color, const dk::geom::plane& plane, const glm::dvec3& right) 
{
	return RGBADrawData(Primitive::Lines, {
		RGBAVertex(plane.transform(edge[0], right), color),
		RGBAVertex(plane.transform(edge[1], right), color)
	});
}

// @brief Draws ray as a line with a point at it's origin
[[nodiscard]] std::vector<RGBADrawData> draw(const dk::geom::ray3& ray, const glm::vec4& color)
{
	return std::vector<RGBADrawData>{
		RGBADrawData(Primitive::Points, { RGBAVertex(ray.origin, color) }),
		RGBADrawData(Primitive::Lines, { 
			RGBAVertex(ray.origin, color),
			RGBAVertex(ray.origin + ray.direction, color),
		})
	};
}

// @brief Draws edges of the bounding box
[[nodiscard]] RGBADrawData draw(const dk::geom::aabb3& aabb, const glm::vec4& color) 
{
	const auto a = aabb.min + aabb.max * glm::dvec3(0, 0, 0);
	const auto b = aabb.min + aabb.max * glm::dvec3(1, 0, 0);
	const auto c = aabb.min + aabb.max * glm::dvec3(1, 0, 1);
	const auto d = aabb.min + aabb.max * glm::dvec3(0, 0, 1);
	const glm::dvec3 up(0, 1, 0);

	RGBADrawData res(Primitive::Lines, {});
	res << draw(dk::geom::edge3{a     , a + up}, color)
		<< draw(dk::geom::edge3{b     , a + up}, color)
		<< draw(dk::geom::edge3{c     , a + up}, color)
		<< draw(dk::geom::edge3{d     , a + up}, color)
		<< draw(dk::geom::edge3{a     , b     }, color)
		<< draw(dk::geom::edge3{b     , c     }, color)
		<< draw(dk::geom::edge3{c     , d     }, color)
		<< draw(dk::geom::edge3{d     , a     }, color)
		<< draw(dk::geom::edge3{a + up, b + up}, color)
		<< draw(dk::geom::edge3{b + up, c + up}, color)
		<< draw(dk::geom::edge3{c + up, d + up}, color)
		<< draw(dk::geom::edge3{d + up, a + up}, color);
	return res;
}

[[nodiscard]] RGBADrawData draw(
	const dk::geom::trig3& trig,
	const glm::vec4&       v1Color,
	const glm::vec4&       v2Color,
	const glm::vec4&       v3Color)
{
	return RGBADrawData(Primitive::Lines, {
		RGBAVertex(trig.vertices[0], v1Color),
		RGBAVertex(trig.vertices[1], v2Color),
		RGBAVertex(trig.vertices[1], v2Color),
		RGBAVertex(trig.vertices[2], v3Color),
		RGBAVertex(trig.vertices[2], v3Color),
		RGBAVertex(trig.vertices[0], v1Color),
	});
}

[[nodiscard]] RGBADrawData draw(
	const dk::geom::trig3& trig,
	const glm::vec4& color)
{
	return draw(trig, color, color, color);
}

[[nodiscard]] RGBADrawData draw(
	const dk::geom::trig2& trig,
	const glm::vec4&       v1Color,
	const glm::vec4&       v2Color,
	const glm::vec4&       v3Color, 
	const dk::geom::plane& plane, 
	const glm::dvec3&      right)
{
	return draw(dk::geom::trig3(trig, plane, right), v1Color, v2Color, v3Color);
}

[[nodiscard]] RGBADrawData draw(
	const dk::geom::trig2& trig,
	const glm::vec4&       color, 
	const dk::geom::plane& plane, 
	const glm::dvec3&      right)
{
	return draw(trig, color, color, color, plane, right);
}

[[nodiscard]] RGBADrawData draw(
	const dk::geom::polygon2& polygon, 
	const glm::vec4&          outerColor, 
	const glm::vec4&          holeColor, 
	const dk::geom::plane&    plane, 
	const glm::dvec3&         right)
{
	RGBADrawData res(Primitive::Lines, {});

	const auto drawEdgeLoop = [&](const std::vector<glm::dvec2>& edgeLoop, const glm::vec4& color) 
	{
		for (int i = 0; i <= edgeLoop.size(); ++i)
			res << draw(dk::geom::edge2{ edgeLoop.at(i), edgeLoop.at((i + 1) % edgeLoop.size()) }, color, plane, right);
	};

	drawEdgeLoop(polygon.vertices, outerColor);

	for (auto& hole : polygon.holes)
		drawEdgeLoop(hole, outerColor);

	return res;
}

}
