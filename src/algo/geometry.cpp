#include <devkit/algo/geometry.h>
#include <devkit/gfx/vertex.h>

#include <poly2tri/poly2tri.h>
#include <algo/polypartition/polypartition.h>

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

double dk::geom::signedTriangle2Area(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c)
{
    const double ax = b.x - a.x;
    const double ay = b.y - a.y;
    const double bx = c.x - a.x;
    const double by = c.y - a.y;
    return bx * ay - ax * by;
}

double dk::geom::ray2::intersectProjection(const glm::dvec2& point) const
{
    const glm::dvec2 pointVector = point - origin;
    const double edgeLengthSquared = glm::dot(direction, direction);
    if (edgeLengthSquared == 0)
        return 0;
    return glm::dot(pointVector, direction) / edgeLengthSquared;
}

double dk::geom::ray3::intersect(const plane& plane) const
{
    const double denom = glm::dot(direction, plane.normal);
    if (std::abs(denom) < 1e-10) {
        // Ray is parallel to the plane
        return std::numeric_limits<double>::infinity(); // or NaN
    }

    const glm::dvec3 diff = plane.point - origin;
    const double t = glm::dot(diff, plane.normal) / denom;
    return t;
}

glm::dvec3 dk::geom::plane::transform(const glm::dvec2& _point, const glm::dvec3& right) const
{
    const auto up = glm::cross(normal, right);
    const auto true_right   = glm::cross(normal, up);
    return true_right * _point.x + up * _point.y + point;
}


std::vector<dk::geom::polygon2> dk::geom::polygon2::convexDecomp() const
{
    TPPLPolyList polygon;

    // Init polygon
    TPPLPoly outerPolygon;
    outerPolygon.Init(vertices.size());
    for (int i = 0; i < vertices.size(); ++i)
        outerPolygon[vertices.size() - i - 1] = { vertices.at(i).x, vertices.at(i).y };
    polygon.push_back(outerPolygon);

    // Init holes
    for (const auto& hole : holes) { 
        TPPLPoly holePolygon;
        holePolygon.Init(hole.size());
        holePolygon.SetHole(true);
        //holePolygon.SetOrientation(TPPL_ORIENTATION_CW);
        for (int i = 0; i < hole.size(); ++i)
            holePolygon[hole.size() - i - 1] = { hole.at(i).x, hole.at(i).y };

        polygon.push_back(holePolygon);
    }

    TPPLPartition partitioner;
    TPPLPolyList  holeFreePolys;
    TPPLPolyList  convexParts;

    if (!partitioner.RemoveHoles(&polygon, &holeFreePolys))
        return {};

    partitioner.ConvexPartition_HM(&holeFreePolys, &convexParts);

    std::vector<polygon2> res;
    for (const auto& part : convexParts) {
        polygon2 partPoly;
        partPoly.vertices.resize(part.GetNumPoints());

        for (int i = 0; i < part.GetNumPoints(); ++i)
            partPoly.vertices.at(i) = { part[i].x, part[i].y};

        res.push_back(partPoly);
    }

    return res;
}

glm::dvec2 dk::geom::polygon2::centroid() const
{
    const auto& pts = vertices;
    if (pts.size() < 3)
        return glm::dvec2(0.0); // Degenerate polygon

    double signedArea = 0.0;
    double cx = 0.0;
    double cy = 0.0;

    for (size_t i = 0; i < pts.size(); ++i) {
        const auto& p0 = pts[i];
        const auto& p1 = pts[(i + 1) % pts.size()];

        double a = p0.x * p1.y - p1.x * p0.y;
        signedArea += a;
        cx += (p0.x + p1.x) * a;
        cy += (p0.y + p1.y) * a;
    }

    signedArea *= 0.5;
    if (std::abs(signedArea) < 1e-10)
        return glm::dvec2(0.0); // Area is too small, possibly degenerate

    cx /= (6.0 * signedArea);
    cy /= (6.0 * signedArea);
    return glm::dvec2(cx, cy);
}

std::vector<dk::geom::polygon2> dk::geom::polygon2::triangulate() const
{
    // Create Poly2Tri points for the outer boundary
    std::vector<p2t::Point*> boundary;
    for (const auto& vertex : vertices)
        boundary.push_back(new p2t::Point(vertex.x, vertex.y));

    // Create a CDT (Constrained Delaunay Triangulation) object
    p2t::CDT cdt(boundary);

    // Add holes
    std::vector<std::vector<p2t::Point*>> p2tHoles;
    for (auto& hole : holes) {
        p2tHoles.emplace_back();
        for (auto& vertex : hole)
            p2tHoles.back().push_back(new p2t::Point(vertex.x, vertex.y));
        cdt.AddHole(p2tHoles.back());
    }

    // Perform the triangulation
    cdt.Triangulate();

    // Retrieve the triangles
    std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();
    std::vector<polygon2> result;

    for (p2t::Triangle* tri : triangles) {
        polygon2 triangle;
        for (int i = 0; i < 3; ++i) {
            p2t::Point* point = tri->GetPoint(i);
            triangle.vertices.push_back(glm::dvec2(static_cast<float>(point->x), static_cast<float>(point->y)));
        }
        result.push_back(triangle);
    }

    // Clean up
    for (p2t::Point* point : boundary) {
        delete point;
    }
    for (const auto& hole : p2tHoles) {
        for (const auto& vertex : hole) {
            delete vertex;
        }
    }

    return result;
}

bool dk::geom::circle2::intersects(const edge2 & edge) const
{
    const auto closest = geom::closestPoint(edge, center);
    return glm::distance(center, closest) <= radius;
}

void dk::geom::bbox2::include(const glm::dvec2& point)
{
    min.x = glm::min(min.x, point.x);
    min.y = glm::min(min.y, point.y);
    max.x = glm::max(max.x, point.x);
    max.y = glm::max(max.y, point.y);
}

double dk::geom::trig2::signedArea() const
{
    return signedTriangle2Area(vertices[0], vertices[1], vertices[2]);
}
