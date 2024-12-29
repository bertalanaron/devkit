#pragma once 
#include <set>
#include <queue>

#include "devkit/util.h"

#ifndef DEVKIT_NOGRAPHICS
#include "devkit/graphics.h"
#endif // !DEVKIT_NOGRAPHICS

namespace NS_DEVKIT {

struct plane {
	glm::dvec3 point;
	glm::dvec3 normal;

	plane(const glm::dvec3& _point, const glm::dvec3& _normal)
		: point(_point), normal(_normal)
	{ }
};

struct aabb3 {
	glm::dvec3 min;
	glm::dvec3 max;
};

using edge2 = std::array<glm::dvec2, 2>;
using edge3 = std::array<glm::dvec2, 3>;

#ifndef DEVKIT_NOGRAPHICS

template <typename T>
concept OutputStream = std::same_as<T, PrimitiveStream> || std::same_as<T, ConcurrentStream>;

struct edge2_graphic {
	const edge2& edge;
	plane        plane;
	glm::vec4    color0;
	glm::vec4    color1;
};

inline edge2_graphic draw(const edge2& edge, const plane& plane, const glm::vec4& color) 
{
	return edge2_graphic{ edge, plane, color, color };
}

template <OutputStream Stream>
Stream& operator<<(Stream& stream, const edge2_graphic& edge)
{
	glm::dvec3 u = glm::normalize(glm::cross(edge.plane.normal, glm::dvec3(0.0, 1.0, 0.0)));
	if (glm::length(u) < 1e-6)
		u = glm::normalize(glm::cross(edge.plane.normal, glm::dvec3(1.0, 0.0, 0.0)));
	glm::dvec3 v = glm::cross(edge.plane.normal, u);

	stream << primitives::LineGradient{ 
		edge.plane.point + edge.edge.at(0).x * u + edge.edge.at(0).y * v,
		edge.color0,
		edge.plane.point + edge.edge.at(0).x * u + edge.edge.at(0).y * v, 
		edge.color1 };

	return stream;
}

#endif // !DEVKIT_NOGRAPHICS

double distance(const glm::dvec2& point, const edge2& edge);
inline double distance(const edge2& edge, const glm::dvec2& point) { return distance(point, edge); }

double distance(const edge2& a, const edge2& b);

struct trig2 {
	std::array<glm::dvec2, 3> vertices;

	std::array<edge2, 3> edges() const;
};

struct trig3 {
	std::array<glm::dvec2, 3> vertices;

	trig3() = default;
	trig3(const trig2& trig, const plane& plane);

	trig3(const std::array<glm::dvec2, 3>& _vertices)
		: vertices(_vertices)
	{ }

	std::array<edge3, 3> edges() const;
};

struct ray3 {
	glm::dvec3 origin;
	glm::dvec3 direction;

	struct hit {
		double     t;
		glm::dvec3 position;

		operator bool() const;
	};

	ray3() = default;
	ray3(const glm::dvec3& _origin, const glm::dvec3& _direction)
		: origin(_origin), direction(_direction)
	{ }

	hit intersect(const plane& plane) const;
	hit intersect(const trig3& trig)  const;
};

struct polygon2 {
	std::vector<glm::dvec2>              vertices;
	std::vector<std::vector<glm::dvec2>> holes;

	std::vector<trig2> triangulate() const;

	// Convert the polygon to list of convex polygons
	std::vector<polygon2> convexDecomp() const;

	bool isPointInside(const glm::dvec2& point) const;
};

}

#ifndef DEVKIT_NOGRAPHICS
namespace NS_DEVKIT {

struct trig2_graphic {
	const trig2& trig;
	plane        plane;
	glm::vec4    color;
};

struct trig3_graphic {
	const trig3& trig;
	glm::vec4    color;
};

template <OutputStream Stream>
Stream& operator<<(Stream& stream, const trig3_graphic& trig)
{
	stream << primitives::Line{ trig.trig.vertices.at(0), trig.trig.vertices.at(1), trig.color }
		   << primitives::Line{ trig.trig.vertices.at(1), trig.trig.vertices.at(2), trig.color }
		   << primitives::Line{ trig.trig.vertices.at(2), trig.trig.vertices.at(0), trig.color };
	return stream;
}
	
}
#endif // !DEVKIT_NOGRAPHICS

namespace NS_DEVKIT {

struct perlin_noise2 {
	int    numOctaves = 7;
	double persistence = 0.5;
	int    primeIndex = 0;

	double operator()(glm::vec2 coord) const;

private:
	double noise(int i, int x, int y) const;
	double smoothedNoise(int i, int x, int y) const;
	double interpolate(double a, double b, double x) const;
	double interpolatedNoise(int i, double x, double y) const;
};

}

	/*==============================================================+
	 *                                                              *
	 *		FLUD FILL                                               *
	 *                                                              *
	 +==============================================================*/

namespace NS_DEVKIT {

template <typename Fnc>
concept fludfill_getter = requires(Fnc get, int x, int y) {
	{ get(x, y) } -> std::same_as<bool>;
};
template <typename Fnc>
concept fludfill_setter = requires(Fnc set, int x, int y, int island) {
	{ set(x, y, island) };
};

template <fludfill_getter FnGetter, fludfill_setter FnSetter>
int fludFill(glm::ivec2 offset, glm::ivec2 size, FnGetter& get, FnSetter& set, bool allowDiagonals = false) {
	int count = 0;
	std::vector<bool> visited(size.x * size.y);

	for (int y = 0; y < size.y; ++y) for (int x = 0; x < size.x; ++x) {
		// Continue if empty
		if (!get(x + offset.x, y + offset.y) || visited.at(y * size.x + x))
			continue;
		++count;

		// Setup queue for neighbours
		std::queue<glm::ivec2> q;
		q.push({ x, y });

		// While the visited cells have unvisited neighbours add them to the region
		while (!q.empty()) {
			glm::ivec2 c = q.front(); q.pop();
			set(c.x + offset.x, c.y + offset.y, count);

			// Get neighbours based on whether diagonals are allowed
			static const std::vector<glm::ivec2> directionsAllowDiags = { 
				{1,0}, {-1,0}, {0,1}, {0,-1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
			static const std::vector<glm::ivec2> directionsDontAllowDiags = { {1,0}, {-1,0}, {0,1}, {0,-1} };
			const std::vector<glm::ivec2>& directinos = allowDiagonals ? directionsAllowDiags : directionsDontAllowDiags;

			// Check neighbours
			for (const glm::ivec2& d : directinos) {
				int nx = c.x + d.x;
				int ny = c.y + d.y;

				if (nx >= 0 && ny >= 0 && nx < size.x && ny < size.y
					&& get(nx + offset.x, ny + offset.y) && !visited.at(ny * size.x + nx))
				{
					q.push({ nx, ny });
					visited.at(ny * size.x + nx) = true;
				}
			}
		}
	}
	return count;
}

}

	/*==============================================================+
	*                                                               *
	*		MARCHING SQUARES                                        *
	*                                                               *
	+===============================================================*/

namespace NS_DEVKIT {

template <typename Fnc>
concept marchingsquares_getter = requires(Fnc get, int x, int y) {
	{ get(x, y) } -> std::same_as<bool>;
};

template <typename StreamType>
struct MarchingsquaresDebug {
	StreamType               out;
	std::array<glm::vec4, 2> colors { dk::colors::white, dk::colors::black };
};

#ifndef DEVKIT_NOGRAPHICS
template <OutputStream Stream>
struct marchingsquares_debug {
	Stream                   out;
	std::array<glm::vec4, 2> edgeGradient;
};
#endif // !DEVKIT_NOGRAPHICS

// \param get bool(int x, int y) Should be able to handle out of bounds coordinates
template <marchingsquares_getter FnGetter
#ifndef DEVKIT_NOGRAPHICS
	, OutputStream Stream
#endif // !DEVKIT_NOGRAPHICS
>
polygon2 marchingSquares(
	glm::ivec2	                        offset,
	glm::ivec2                          size, 
	FnGetter                            get, 
	bool	                            allowDiagonals = false,
#ifndef DEVKIT_NOGRAPHICS
	std::optional<std::reference_wrapper<marchingsquares_debug<Stream>>> 
	debug = std::nullopt
#endif // !DEVKIT_NOGRAPHICS
	) {
	// Remove non corner vertices from edge loop
	auto connectEdges = [](
		const std::unordered_map<glm::dvec2, glm::dvec2>& input, 
		const glm::dvec2&                                 first, 
		std::set<glm::dvec2>&                             visited, 
		std::vector<glm::dvec2>&                          output) 
		{
			auto it = input.find(first);
			glm::dvec2 prev = it->first;
			for (it = input.find(it->second); 
				!visited.contains(it->first); 
				prev = it->first, it = input.find(it->second)) 
			{
				visited.insert(it->first);

				glm::dvec2 d1 = glm::normalize(it->first - prev);
				glm::dvec2 d2 = glm::normalize(it->second - it->first);

				if (glm::dot(d1, d2) < 0.99)
					output.push_back(it->first);
			}
		};

	// Lookup table for the Marching Squares algorithm
	static constexpr int s_marchingSquaresLookupAllowDiags[16][4] = {
		{ -1, -1, -1, -1 }, {  1,  0, -1, -1 }, {  2,  1, -1, -1 }, {  2,  0, -1, -1 },
		{  3,  2, -1, -1 }, {  3,  0,  1,  2 }, {  3,  1, -1, -1 }, {  3,  0, -1, -1 },
		{  0,  3, -1, -1 }, {  1,  3, -1, -1 }, {  0,  1,  2,  3 }, {  2,  3, -1, -1 },
		{  0,  2, -1, -1 }, {  1,  2, -1, -1 }, {  0,  1, -1, -1 }, { -1, -1, -1, -1 }
	};
	// When not allowing diagonals one region can result in multiple polygons.
	// Correct implementation should prevent most diagonals at the region identification step.
	// Use of this table is only needed because of regions whos cells are not only connected trhough diagonals. 
	static constexpr int s_marchingSquaresLookupDontAllowDiags[16][4] = {
		{ -1, -1, -1, -1 }, {  1,  0, -1, -1 }, {  2,  1, -1, -1 }, {  2,  0, -1, -1 },
		{  3,  2, -1, -1 }, {  1,  0,  3,  2 }, {  3,  1, -1, -1 }, {  3,  0, -1, -1 },
		{  0,  3, -1, -1 }, {  1,  3, -1, -1 }, {  0,  3,  2,  1 }, {  2,  3, -1, -1 },
		{  0,  2, -1, -1 }, {  1,  2, -1, -1 }, {  0,  1, -1, -1 }, { -1, -1, -1, -1 }
	};
	const auto& lookup = (allowDiagonals) 
		? s_marchingSquaresLookupAllowDiags
		: s_marchingSquaresLookupDontAllowDiags;

	static const std::array<glm::dvec2, 4> vertexOffsets { 
		glm::dvec2{ .5f, 0.f }, glm::dvec2{ 0.f, .5f }, glm::dvec2{ .5f, 1.f }, glm::dvec2{ 1.f, .5f } };

	std::unordered_map<glm::dvec2, glm::dvec2> edges{};
	bool firstVertFound = false;
	glm::vec2 firstVert;

	// Trace outer boundary square
	glm::ivec2 direction{ 1, 0 };
	glm::ivec2 coord{ offset.x, offset.y };
	for (bool first = true;;first = false,coord += direction) {
		if (glm::ivec2{ offset.x + size.x, offset.y          } == coord) direction = { 0, 1 };
		if (glm::ivec2{ offset.x + size.x, offset.y + size.y } == coord) direction = { -1, 0 };
		if (glm::ivec2{ offset.x, offset.y + size.y          } == coord) direction = { 0, -1 };
		if (glm::ivec2{ offset.x, offset.y                   } == coord && !first) break;

		constexpr static std::array<glm::dvec2, 4> s_boundaryOffsetLookup {
			glm::dvec2{ -1, -1 }, glm::dvec2{ 0, .5 }, glm::dvec2{ .5, 1. }, glm::dvec2{ 0., 1. } };

		int boundaryOffsetIndex = 0;
		auto next = coord + direction;
		if (get(coord.x, coord.y)) boundaryOffsetIndex |= 1;
		if (get(next.x, next.y)) boundaryOffsetIndex |= 2;

		const auto& factor = s_boundaryOffsetLookup.at(boundaryOffsetIndex);
		if (factor.x < 0)
			continue;

		glm::dvec2 pos = glm::dvec2(coord) + glm::dvec2{ 0.5, 0.5 };
		if (!firstVertFound) {
			firstVertFound = true;
			firstVert = pos + glm::dvec2(direction) * factor.y;
		}
		edges.insert({ pos + glm::dvec2(direction) * factor.y, pos + glm::dvec2(direction) * factor.x });
	}

	// Construct edges using marching squares
	for (int y = offset.y; y < size.y + offset.y; ++y) for (int x = offset.x; x < size.x + offset.x; ++x) {
		int index = 0;
		if (get(x + 0, y + 0)) index |= 1;
		if (get(x + 0, y + 1)) index |= 2;
		if (get(x + 1, y + 1)) index |= 4;
		if (get(x + 1, y + 0)) index |= 8;

		for (int i = 0; i < 2; ++i) {
			if (lookup[index][i * 2] == -1) 
				continue;

			if (!firstVertFound) {
				firstVertFound = true;
				firstVert = glm::dvec2(x + .5, y + .5) + vertexOffsets[lookup[index][i * 2 + 0]];
			}
			edges.insert({
				glm::dvec2(x + .5, y + .5) + vertexOffsets[lookup[index][i * 2 + 0]],
				glm::dvec2(x + .5, y + .5) + vertexOffsets[lookup[index][i * 2 + 1]]
				});
		}
	}

	// Visualize edges
#ifndef DEVKIT_NOGRAPHICS
	if (debug.has_value()) 
	{
		for (auto& [from, to] : edges) 
			debug.value().get().out << dk::primitives::LineGradient{
				{ from.x, 1., from.y }, debug.value().get().edgeGradient.at(0), 
				{ to.x, 1., to.y }    , debug.value().get().edgeGradient.at(1) };
	}
#endif // !DEVKIT_NOGRAPHICS

	// Construct polygon from edges
	polygon2 polygon;
	if (edges.empty())
		return polygon;
	std::set<glm::dvec2> visitedEdges{};
	// Trace polygon boundary
	connectEdges(edges, firstVert, visitedEdges, polygon.vertices);
	// Identify and trace holes inside the region and store them in polygon.holes
	for (auto& edge : edges) {
		if (visitedEdges.contains(edge.first))
			continue;

		std::vector<glm::dvec2> hole{};
		connectEdges(edges, edge.first, visitedEdges, hole);

		polygon.holes.push_back(hole);
	}

	return polygon;
}

}
