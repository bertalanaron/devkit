#include <devkit/algo/geometry.h>
#include <devkit/gfx/vertex.h>

dk::geom::plane dk::geom::plane::operator+(double s) const
{
	return plane(point + normal * s, normal);
}

bool dk::geom::isIntersecting(double t)
{
    return t < std::numeric_limits<double>::infinity() && t >= 0;
}

glm::dvec2 dk::geom::projectPoint(const edge2& edge, const glm::dvec2& point)
{
    return edge.at(0) + ray2(edge[0], edge[1] - edge[0]).intersectProjection(point) * (edge.at(1) - edge.at(0));
}

glm::dvec2 dk::geom::closestPoint(const edge2& edge, const glm::dvec2& point)
{
    glm::dvec2 edgeVector = edge.at(1) - edge.at(0);
    double t = ray2(edge[0], edge[1] - edge[0]).intersectProjection(point);
    t = glm::clamp(t, 0.0, 1.0);
    return edge.at(0) + t * edgeVector;
}

double dk::geom::distance(const edge2& edge, const glm::dvec2& point)
{
    return glm::distance(closestPoint(edge, point), point);
}

double dk::geom::distance(const glm::dvec2& point, const edge2& edge)
{
    return distance(edge, point);
}

double dk::geom::distance(const edge2& edge1, const edge2& edge2)
{
    double d1 = distance(edge1.at(0), edge2),
           d2 = distance(edge1.at(1), edge2),
           d3 = distance(edge2.at(0), edge1),
           d4 = distance(edge2.at(1), edge1);
    return std::min(std::min(d1, d2), std::min(d3, d4));
}

double dk::geom::ray2::intersectProjection(const glm::dvec2& point) const
{
    const glm::dvec2 pointVector = point - origin;
    const double edgeLengthSquared = glm::dot(direction, direction);
    if (edgeLengthSquared == 0)
        return 0;
    return glm::dot(pointVector, direction) / edgeLengthSquared;
}

glm::dvec3 dk::geom::plane::transform(const glm::dvec2& _point, const glm::dvec3& right) const
{
    const auto up = glm::cross(normal, right);
    const auto true_right   = glm::cross(normal, up);
    return true_right * _point.x + up * _point.y + point;
}
