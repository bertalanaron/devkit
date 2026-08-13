#pragma once
#include <devkit/common/utils.h>

namespace dk::geom {

constexpr glm::dvec2 Origin2(0, 0);
constexpr glm::dvec3 Origin3(0, 0, 0);

namespace axis {

constexpr glm::dvec3 X = { 1, 0, 0 };
constexpr glm::dvec3 Y = { 0, 1, 0 };
constexpr glm::dvec3 Z = { 0, 0, 1 };

}

inline glm::dvec2 xy(const glm::dvec3& vec) { return glm::dvec2(vec.x, vec.y); }
inline glm::ivec2 xy(const glm::ivec3& vec) { return glm::ivec2(vec.x, vec.y); }
inline glm::dvec2 xz(const glm::dvec3& vec) { return glm::dvec2(vec.x, vec.z); }
inline glm::ivec2 xz(const glm::ivec3& vec) { return glm::ivec2(vec.x, vec.z); }
inline glm::dvec2 yz(const glm::dvec3& vec) { return glm::dvec2(vec.y, vec.z); }
inline glm::ivec2 yz(const glm::ivec3& vec) { return glm::ivec2(vec.y, vec.z); }

// Forward declarations
struct aabb3;

bool isIntersecting(double t);

using edge2 = std::array<glm::dvec2, 2>;
using edge3 = std::array<glm::dvec3, 2>;

// @brief Edge instance initialized with zeros
constexpr edge2 nullEdge2 = edge2{ glm::dvec2(0,0)  , glm::dvec2(0,0)   };
// @brief Edge instance initialized with zeros
constexpr edge3 nullEdge3 = edge3{ glm::dvec3(0,0,0), glm::dvec3(0,0,0) };

enum class orientation { ClockWise, CounterClockWise };

// @brief Projects point onto the line defined by the edge
glm::dvec2 projectPoint(const edge2& edge, const glm::dvec2& point);

// @brief Clamps the point's projection onto the edge's length
glm::dvec2 closestPoint(const edge2& edge, const glm::dvec2& point);

double distance(const edge2& edge, const glm::dvec2& point);
double distance(const glm::dvec2& point, const edge2& edge);
double distance(const edge2& edge1, const edge2& edge2);

// @brief clamp length of vector
auto clampLength(const auto& vec, auto min, auto max)
{
	const auto length = glm::length(vec);
	if (length < min)
		return glm::normalize(vec) * min;
	if (length > max)
		return glm::normalize(vec) * max;
	return vec;
}

// @brief 3D plane defined by a point and a normal vector
struct plane {
	glm::dvec3 point;
	glm::dvec3 normal;

	constexpr plane(const glm::dvec3& _point, const glm::dvec3& _normal)
		: point(_point), normal(_normal)
	{ }

	constexpr plane(const glm::dvec3& _normal)
		: point(0, 0, 0), normal(_normal)
	{ }

	// @brief Move the plane in the direction of the normal by normal * s
	plane operator+(double s) const;

	// @brief Creates a plane crossing (0,0,0) with the normal pointing in the direction of the X axis
	static constexpr plane X() { return plane(dk::geom::axis::X); }
	// @brief Creates a plane crossing (0,0,0) with the normal pointing in the direction of the Y axis
	static constexpr plane Y() { return plane(dk::geom::axis::Y); }
	// @brief Creates a plane crossing (0,0,0) with the normal pointing in the direction of the Z axis
	static constexpr plane Z() { return plane(dk::geom::axis::Z); }

	glm::dvec3 project(const glm::vec3& point) const;

	glm::dvec3 transform(const glm::dvec2& point, const glm::dvec3& right) const;
};

double distance(const plane& plane, const glm::dvec3& point);
double distance(const glm::dvec3& point, const plane& plane);

struct ray2 {
	glm::dvec2 origin;
	glm::dvec2 direction;

	// @brief The 
	double intersectProjection(const glm::dvec2& point) const;

	double intersect(const edge2& edge) const;
	std::pair<double, double> intersect(const ray2& ray) const;
};

inline glm::dvec2 intersection(const ray2& ray, const auto& target)
{
	double t = ray.intersect(target);
	return ray.origin + ray.direction * t;
}

struct ray3 {
	glm::dvec3 origin;
	glm::dvec3 direction;

	void includePoint(const glm::dvec3& point);

	double intersect(const plane& plane) const;
	double intersect(const aabb3& aabb) const;
};

inline glm::dvec3 intersection(const ray3& ray, const auto& target)
{
	double t = ray.intersect(target);
	return ray.origin + ray.direction * t;
}

struct bbox2 {
	glm::dvec2 min = glm::dvec2(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
	glm::dvec2 max = glm::dvec2(-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity());

	void include(const glm::dvec2& point);
};

// @brief 3D Axis Aligned Bounding Box 
struct aabb3 {
	glm::dvec3 min;
	glm::dvec3 max;

	double intersect(const ray3& ray) const;
};

struct trig2 {
	std::array<glm::dvec2, 3> vertices;

	trig2() = default;
	trig2(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c)
		: vertices{a, b, c}
	{ }

	double signedArea() const;
};

// @brief Negative for clockwise and positive for counter clockwise
double signedTriangle2Area(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);

struct trig3 {
	std::array<glm::dvec3, 3> vertices;

	trig3() = default;
	trig3(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c)
		: vertices{a, b, c}
	{ }

	trig3(const trig2& trig, const plane& plane, const glm::dvec3& right)
		: vertices{ plane.transform(trig.vertices[0], right),
	                plane.transform(trig.vertices[1], right),
	                plane.transform(trig.vertices[2], right) }
	{ }
};

struct polygon2 {
	using edge_loop_t = std::vector<glm::dvec2>;

	edge_loop_t              vertices;
	std::vector<edge_loop_t> holes;

	std::vector<polygon2> triangulate() const;
	std::vector<polygon2> convexDecomp() const;

	glm::dvec2 centroid() const;
	bool isPointInside(const glm::dvec2& point) const;
};

struct circle2 {
	glm::dvec2 center;
	double     radius;

	bool intersects(const edge2& edge) const;
};

template <typename T>
class grid2 {
public:
	grid2(const glm::ivec2& size)
		: m_size(size)
		, m_data(size.x * size.y)
	{ }

	T& at(const glm::ivec2& coord)
	{ return m_data[coord.y * m_size.x + coord.x]; }

	const T& at(const glm::ivec2& coord) const
	{ return m_data[coord.y * m_size.x + coord.x]; }

	void resize(const glm::ivec2& size)
	{
		std::vector<T> newData(size.x * size.y);
		for (auto y = 0; y < std::min(m_size.y, size.y); ++y)
			for (auto x = 0; x < std::min(m_size.x, size.x); ++x)
				newData.at(y * size.x + x) = m_data.at(y * m_size.x + x);
		m_size = size;
		m_data = std::move(newData);
	}

protected:
	glm::ivec2     m_size;
	std::vector<T> m_data;
};

template <typename T>
class chunked_grid2 {
public:
	chunked_grid2(const glm::ivec2& size, const glm::ivec2& chunkSize)
		: m_size(size)
		, m_data(size.x * size.y)
		, m_chunkSize(chunkSize)
	{ }

	T& at(const glm::ivec2& coord)
	{ return m_data[index(coord)]; }

	const T& at(const glm::ivec2& coord) const
	{ return m_data[index(coord)]; }

	const glm::ivec2& size() const
	{ return m_size; }

	const glm::ivec2& chunkSize() const
	{ return m_chunkSize; }

	//void resize(const glm::ivec2& size)
	//{
	//	std::vector<T> newData(size.x * size.y);
	//	for (auto y = 0; y < std::min(m_size.y, size.y); ++y)
	//		for (auto x = 0; x < std::min(m_size.x, size.x); ++x)
	//			newData.at(y * size.x + x) = m_data.at(y * m_size.x + x);
	//	m_size = size;
	//	m_data = std::move(newData);
	//}

	//void setChunkSize(const glm::ivec2& chunkSize)
	//{ m_chunkSize = chunkSize; }

	glm::ivec2 chunkOf(const glm::ivec2& coord) const
	{ return coord / m_chunkSize; }

protected:
	glm::ivec2     m_size;
	std::vector<T> m_data;
	glm::ivec2     m_chunkSize;

	int index(const glm::ivec2& coord) const
	{ 
		const auto chunkCoord = chunkOf(coord);
		const int chunkBegIndex = chunkCoord.y * m_chunkSize.y * m_size.x + chunkCoord.x * m_chunkSize.x;
		return chunkBegIndex + (coord.y % m_chunkSize.y * m_chunkSize.x + coord.x % m_chunkSize.x);
	}
};

}

template <>
struct std::hash<dk::geom::edge2> {
	std::size_t operator()(const dk::geom::edge2& p) const {
		std::size_t h1 = std::hash<glm::dvec2>{}(p.at(0));
		std::size_t h2 = std::hash<glm::dvec2>{}(p.at(1));
		return h1 ^ (h2 << 1);
	}
};
