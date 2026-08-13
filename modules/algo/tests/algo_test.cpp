#include <devkit/algo/geometry.h>

#include <gtest/gtest.h>

namespace {

TEST(AlgoTest, ComputesDistanceFromPointToEdge)
{
    const dk::geom::edge2 edge = { glm::dvec2(0.0, 0.0), glm::dvec2(10.0, 0.0) };

    EXPECT_DOUBLE_EQ(dk::geom::distance(edge, glm::dvec2(5.0, 3.0)), 3.0);
    EXPECT_DOUBLE_EQ(dk::geom::distance(edge, glm::dvec2(12.0, 0.0)), 2.0);
}

}
