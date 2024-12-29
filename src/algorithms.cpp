#include "devkit/algorithms.h"

//#include "poly2tri.h"
#include "polypartition.h"

using namespace NS_DEVKIT;

ray3::hit ray3::intersect(const plane& plane) const
{
    hit hit{ .t = std::numeric_limits<double>::infinity() };

    double denominator = glm::dot(plane.normal, direction);
    if (std::abs(denominator) < 1e-6) // Ray is parallel
        return hit;

    hit.t = glm::dot(plane.point - origin, plane.normal) / denominator;
    hit.position = origin + hit.t * direction;

	return hit;
}

ray3::hit::operator bool() const
{
    return t < std::numeric_limits<double>::infinity() && t >= 0;
}

std::array<edge2, 3> trig2::edges() const
{
    return { edge2{ vertices.at(0), vertices.at(1) },
             edge2{ vertices.at(1), vertices.at(2) },
             edge2{ vertices.at(2), vertices.at(0) } };
}

trig3::trig3(const trig2& trig, const plane& plane)
{
    glm::dvec3 u = glm::normalize(glm::cross(plane.normal, glm::dvec3(0.0, 1.0, 0.0)));
    if (glm::length(u) < 1e-6)
        u = glm::normalize(glm::cross(plane.normal, glm::dvec3(1.0, 0.0, 0.0)));
    glm::dvec3 v = glm::cross(plane.normal, u);

    for (size_t i = 0; i < 3; ++i) {
        const glm::dvec2& vertex2D = trig.vertices[i];
        vertices[i] = plane.point + vertex2D.x * u + vertex2D.y * v;
    }
}

std::array<edge3, 3> trig3::edges() const
{
    return { edge3{ vertices.at(0), vertices.at(1) },
             edge3{ vertices.at(1), vertices.at(2) },
             edge3{ vertices.at(2), vertices.at(0) } };
}

inline constexpr size_t g_perlinMaxPrimeIndex = 10;
inline constexpr int    g_perlinPrimes[g_perlinMaxPrimeIndex][3] = {
    { 995615039, 600173719, 701464987 },
    { 831731269, 162318869, 136250887 },
    { 174329291, 946737083, 245679977 },
    { 362489573, 795918041, 350777237 },
    { 457025711, 880830799, 909678923 },
    { 787070341, 177340217, 593320781 },
    { 405493717, 291031019, 391950901 },
    { 458904767, 676625681, 424452397 },
    { 531736441, 939683957, 810651871 },
    { 997169939, 842027887, 423882827 }
};

double perlin_noise2::operator()(glm::vec2 coord) const {
    double total = 0,
           frequency = std::pow(2.0, numOctaves),
           amplitude = 1;
    for (int i = 0; i < numOctaves; ++i) {
        frequency /= 2;
        amplitude *= persistence;
        total += interpolatedNoise((primeIndex + i) % g_perlinMaxPrimeIndex,
            coord.x / frequency, coord.y / frequency) * amplitude;
    }
    return total / frequency;
}

inline double perlin_noise2::noise(int i, int x, int y) const {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    int a = g_perlinPrimes[i][0], b = g_perlinPrimes[i][1], c = g_perlinPrimes[i][2];
    int t = (n * (n * n * a + b) + c) & 0x7fffffff;
    return 1.0 - (double)(t) / 1073741824.0;
}

inline double perlin_noise2::smoothedNoise(int i, int x, int y) const {
    double corners = (noise(i, x - 1, y - 1) + noise(i, x + 1, y - 1) +
               noise(i, x - 1, y + 1) + noise(i, x + 1, y + 1)) / 16,
           sides = (noise(i, x - 1, y) + noise(i, x + 1, y) +
               noise(i, x, y - 1) + noise(i, x, y + 1)) / 8,
           center = noise(i, x, y) / 4;
    return corners + sides + center;
}

inline double perlin_noise2::interpolate(double a, double b, double x) const {
    double ft = x * 3.1415927,
        f = (1 - std::cos(ft)) * 0.5;
    return  a * (1 - f) + b * f;
}

inline double perlin_noise2::interpolatedNoise(int i, double x, double y) const {
    int integer_X = x;
    double fractional_X = x - integer_X;
    int integer_Y = y;
    double fractional_Y = y - integer_Y;

    double v1 = smoothedNoise(i, integer_X, integer_Y),
           v2 = smoothedNoise(i, integer_X + 1, integer_Y),
           v3 = smoothedNoise(i, integer_X, integer_Y + 1),
           v4 = smoothedNoise(i, integer_X + 1, integer_Y + 1),
           i1 = interpolate(v1, v2, fractional_X),
           i2 = interpolate(v3, v4, fractional_X);
    return interpolate(i1, i2, fractional_Y);
}

//std::vector<trig2> polygon2::triangulate() const
//{
//    //// Create Poly2Tri points for the outer boundary
//    //std::vector<p2t::Point*> boundary;
//    //for (const auto& vertex : vertices)
//    //    boundary.push_back(new p2t::Point(vertex.x, vertex.y));
//    //if (boundary.empty()) {
//    //    spdlog::warn("Empty boudary");
//    //    return {};
//    //}
//
//    //// Create a CDT (Constrained Delaunay Triangulation) object
//    //p2t::CDT cdt(boundary);
//
//    //// Add holes
//    //std::vector<std::vector<p2t::Point*>> p2tHoles{};
//    //for (const auto& hole : holes) {
//    //    std::vector<p2t::Point*> holePoints;
//    //    for (const auto& vertex : hole) {
//    //        holePoints.push_back(new p2t::Point(vertex.x, vertex.y));
//    //    }
//    //    if (holePoints.size() < 3) {
//    //        spdlog::warn("Empty hole");
//    //        continue;
//    //    }
//    //    cdt.AddHole(holePoints);
//    //    p2tHoles.push_back(holePoints);
//    //}
//
//    //// Perform the triangulation
//    //cdt.Triangulate();
//
//    //// Retrieve the triangles
//    //std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();
//    //std::vector<trig2> result;
//
//    //for (p2t::Triangle* tri : triangles) {
//    //    trig2 triangle;
//    //    for (int i = 0; i < 3; ++i) {
//    //        p2t::Point* point = tri->GetPoint(i);
//    //        triangle.vertices.at(i) = glm::vec2(static_cast<float>(point->x), static_cast<float>(point->y));
//    //    }
//    //    result.push_back(triangle);
//    //}
//
//    //// Clean up
//    //for (p2t::Point* point : boundary) {
//    //    delete point;
//    //}
//    //for (const auto& hole : p2tHoles) {
//    //    for (const auto& point : hole) {
//    //        delete point;
//    //    }
//    //}
//
//    //return result;
//}

double NS_DEVKIT::distance(const glm::dvec2& point, const edge2& edge)
{
    glm::dvec2 edgeVector = edge.at(1) - edge.at(0);
    glm::dvec2 pointVector = point - edge.at(0);

    double edgeLengthSquared = glm::dot(edgeVector, edgeVector);
    double t = glm::dot(pointVector, edgeVector) / edgeLengthSquared;

    t = glm::clamp(t, 0.0, 1.0);

    glm::dvec2 closestPoint = edge.at(0) + t * edgeVector;

    return glm::distance(point, closestPoint);
}

double NS_DEVKIT::distance(const edge2& a, const edge2& b)
{
    double d1 = distance(a.at(0), b),
           d2 = distance(a.at(1), b),
           d3 = distance(b.at(0), a),
           d4 = distance(b.at(1), a);
    return std::min(std::min(d1, d2), std::min(d3, d4));
}

std::vector<polygon2> NS_DEVKIT::polygon2::convexDecomp() const
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
    TPPLPolyList holeFreePolys;
    TPPLPolyList convexParts;

    if (!partitioner.RemoveHoles(&polygon, &holeFreePolys))
        return {};

    //for (auto& holeFreePoly : holeFreePolys)
    partitioner.ConvexPartition_HM(&holeFreePolys, &convexParts);

    //TPPLPolyList holeFreePolys;
    ////if (!partitioner.ConvexPartition_HM(&polygon, &convexParts))
    ////    throw std::runtime_error("Convex partitioning failed");
    //partitioner.ConvexPartition_HM(&polygon, &convexParts);
    //TPPLPolyList holeFreePolys;
    //partitioner.RemoveHoles(&inputPolys, &holeFreePolys)

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

bool polygon2::isPointInside(const glm::dvec2& point) const
{
    auto isInsideBoundary = [&](const glm::dvec2& point, const std::vector<glm::dvec2>& points) {
        bool inside = false;
        size_t n = points.size();

        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            const glm::dvec2& vi = points[i];
            const glm::dvec2& vj = points[j];

            bool intersect = (vi.y > point.y) != (vj.y > point.y);
            if (intersect && point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x) {
                inside = !inside;
            }
        }
        return inside; };

    if (!isInsideBoundary(point, vertices))
        return false;

    for (const auto& hole : holes) {
        if (isInsideBoundary(point, hole))
            return false;
    }
}
