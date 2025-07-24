#pragma once
#include <devkit/gfx/draw_data.h>
#include <devkit/algo/geometry.h>
#include <devkit/algo/funnel.h>

namespace dk::gfx {

[[nodiscard]] inline RGBADrawData draw(const glm::dvec3& point, const glm::vec4& color) 
{
	return RGBADrawData(Primitive::Points, {
		RGBAVertex(point, color)
	});
}

[[nodiscard]] inline RGBADrawData draw(const glm::dvec2& point, const glm::vec4& color, const dk::geom::plane& plane, const glm::dvec3& right) 
{
	return RGBADrawData(Primitive::Points, {
		RGBAVertex(plane.transform(point, right), color)
	});
}

// @brief Draws a 3d edge using a uniform color
[[nodiscard]] inline RGBADrawData draw(const dk::geom::edge3& edge, const glm::vec4& color1, const glm::vec4& color2) 
{
	return RGBADrawData(Primitive::Lines, {
		RGBAVertex(edge[0], color1),
		RGBAVertex(edge[1], color2)
	});
}

// @brief Draws a 3d edge using a uniform color
[[nodiscard]] inline RGBADrawData draw(const dk::geom::edge3& edge, const glm::vec4& color) 
{
	return draw(edge, color, color);
}

// @brief Draws the 2d edge on a plane using a uniform color: (0,0) is at the plains defined point.
//
// Example: dk::gfx::draw({glm::dvec2{0,0},{1,1}}, dk::colors::red, dk::geom::plane::Y(), dk::geom::axis::Y)
// @param up - The vector which is used to get the 2d axis with in combination with the plane's normal. (right = plane.normal x up)
[[nodiscard]] inline RGBADrawData draw(const dk::geom::edge2& edge, const glm::vec4& color1, const glm::vec4& color2, const dk::geom::plane& plane, const glm::dvec3& right) 
{
	return RGBADrawData(Primitive::Lines, {
		RGBAVertex(plane.transform(edge[0], right), color1),
		RGBAVertex(plane.transform(edge[1], right), color2)
	});
}

[[nodiscard]] inline RGBADrawData draw(const dk::geom::edge2& edge, const glm::vec4& color, const dk::geom::plane& plane, const glm::dvec3& right) 
{
	return draw(edge, color, color, plane, right);
}

// @brief Draws ray as a line with a point at it's origin
[[nodiscard]] inline std::vector<RGBADrawData> draw(const dk::geom::ray3& ray, const glm::vec4& color)
{
	return std::vector<RGBADrawData>{
		RGBADrawData(Primitive::Points, { RGBAVertex(ray.origin, color) }),
		RGBADrawData(Primitive::Lines, { 
			RGBAVertex(ray.origin, color),
			RGBAVertex(ray.origin + ray.direction, color),
		})
	};
}

// @brief Draws ray as a line with a point at it's origin
[[nodiscard]] inline std::vector<RGBADrawData> draw(const dk::geom::ray2& ray, const glm::vec4& color, const dk::geom::plane& plane, const glm::dvec3& right)
{
	return std::vector<RGBADrawData>{
		RGBADrawData(Primitive::Points, { RGBAVertex(plane.transform(ray.origin, right), color) }),
			RGBADrawData(Primitive::Lines, { 
			RGBAVertex(plane.transform(ray.origin, right), color),
			RGBAVertex(plane.transform(ray.origin + ray.direction, right), color),
		})
	};
}

// @brief Draws edges of the bounding box
[[nodiscard]] inline RGBADrawData draw(const dk::geom::aabb3& aabb, const glm::vec4& color) 
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

[[nodiscard]] inline RGBADrawData draw(
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

[[nodiscard]] inline RGBADrawData draw(
	const dk::geom::trig3& trig,
	const glm::vec4& color)
{
	return draw(trig, color, color, color);
}

[[nodiscard]] inline RGBADrawData draw(
	const dk::geom::trig2& trig,
	const glm::vec4&       v1Color,
	const glm::vec4&       v2Color,
	const glm::vec4&       v3Color, 
	const dk::geom::plane& plane, 
	const glm::dvec3&      right)
{
	return draw(dk::geom::trig3(trig, plane, right), v1Color, v2Color, v3Color);
}

[[nodiscard]] inline RGBADrawData draw(
	const dk::geom::trig2& trig,
	const glm::vec4&       color, 
	const dk::geom::plane& plane, 
	const glm::dvec3&      right)
{
	return draw(trig, color, color, color, plane, right);
}

[[nodiscard]] inline RGBADrawData draw(
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

// @brief Draw 2d circle on plane
[[nodiscard]] inline RGBADrawData draw(
	const dk::geom::circle2& circle,
	int                      vertexCount, 
	const glm::vec4&         color,
	const dk::geom::plane&   plane, 
	const glm::dvec3&        right)
{
	RGBADrawData res(Primitive::Lines, {});

	double angleIncrementRad = 2.0 * glm::pi<double>() / (double)vertexCount;
	double angle = 0;
	for (int i = 0; i <= vertexCount; ++i, angle += angleIncrementRad)
		res << draw(dk::geom::edge2{ 
			circle.center + circle.radius * glm::dvec2(glm::cos(angle), glm::sin(angle)), 
			circle.center + circle.radius * glm::dvec2(glm::cos(angle + angleIncrementRad), glm::sin(angle + angleIncrementRad))
		}, color, plane, right);

	return res;
}

// @brief Draw 2d circle on plane, auto set vertex count based on radius
[[nodiscard]] inline RGBADrawData draw(
	const dk::geom::circle2& circle,
	const glm::vec4&         color,
	const dk::geom::plane&   plane, 
	const glm::dvec3&        right)
{
	RGBADrawData res(Primitive::Lines, {});

	const int vertexCount = 31 * circle.radius;
	return draw(circle, vertexCount, color, plane, right);
}

[[nodiscard]] inline std::vector<RGBADrawData> draw(
	const algo::Funnel&    funnel, 
	const glm::vec4&       apexColor, 
	const glm::vec4&       leftColor, 
	const glm::vec4&       rightColor, 
	const glm::vec4&       tailColor, 
	const dk::geom::plane& plane, 
	const glm::dvec3&      right)
{
	RGBADrawData leftSide(Primitive::Lines, {});
	if (!funnel.leftQueue.empty())
		leftSide.vertices.push_back(RGBAVertex(plane.transform(funnel.apex, right), leftColor));
	for (int i = 0; !funnel.leftQueue.empty() && i < funnel.leftQueue.size() - 1; ++i) {
		leftSide.vertices.push_back(RGBAVertex(plane.transform(funnel.leftQueue.at(i), right), leftColor));
		leftSide.vertices.push_back(RGBAVertex(plane.transform(funnel.leftQueue.at(i), right), leftColor));
	}
	if (!funnel.leftQueue.empty())
		leftSide.vertices.push_back(RGBAVertex(plane.transform(funnel.leftQueue.at(funnel.leftQueue.size() - 1), right), leftColor));

	RGBADrawData rightSide(Primitive::Lines, {});
	if (!funnel.rightQueue.empty())
		rightSide.vertices.push_back(RGBAVertex(plane.transform(funnel.apex, right), rightColor));
	for (int i = 0; !funnel.rightQueue.empty() && i < funnel.rightQueue.size() - 1; ++i) {
		rightSide.vertices.push_back(RGBAVertex(plane.transform(funnel.rightQueue.at(i), right), rightColor));
		rightSide.vertices.push_back(RGBAVertex(plane.transform(funnel.rightQueue.at(i), right), rightColor));
	}
	if (!funnel.rightQueue.empty())
		rightSide.vertices.push_back(RGBAVertex(plane.transform(funnel.rightQueue.at(funnel.rightQueue.size() - 1), right), rightColor));
	
	RGBADrawData tail(Primitive::Lines, {});
	for (int i = 0; !funnel.tail.empty() && i < funnel.tail.size() - 1; ++i) {
		tail.vertices.push_back(RGBAVertex(plane.transform(funnel.tail.at(i), right), tailColor));
		tail.vertices.push_back(RGBAVertex(plane.transform(funnel.tail.at(i + 1), right), tailColor));
	}
	if (!funnel.tail.empty()) {
		tail.vertices.push_back(RGBAVertex(plane.transform(funnel.tail.at(funnel.tail.size() - 1), right), tailColor));
		tail.vertices.push_back(RGBAVertex(plane.transform(funnel.apex, right), tailColor));
	}

	return std::vector<RGBADrawData>{
		// Apex
		RGBADrawData(Primitive::Points, { RGBAVertex(plane.transform(funnel.apex, right), apexColor) }),
			std::move(leftSide),
			std::move(rightSide),
			std::move(tail),
	};
}

[[nodiscard]] inline std::vector<RGBADrawData> draw(
	const algo::Funnel&    funnel, 
	const glm::vec4&       color, 
	const dk::geom::plane& plane, 
	const glm::dvec3&      right)
{
	return draw(funnel, color, color, color, color, plane, right);
}

struct drawer2d {
	geom::plane plane;
	glm::dvec3  right;

	[[nodiscard]] auto operator()(auto&&... args) const
	{
		return dk::gfx::draw(std::forward<decltype(args)>(args)..., plane, right);
	}
};

}
