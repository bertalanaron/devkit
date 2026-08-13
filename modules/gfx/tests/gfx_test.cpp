#include <devkit/gfx/viewport.h>

#include <gtest/gtest.h>

namespace {

TEST(GfxTest, NormalizesPixelsWithinViewport)
{
    const dk::gfx::Viewport viewport(glm::ivec2(200, 100), glm::ivec2(10, 20), 30);

    const glm::vec2 normalized = viewport.normalize(glm::ivec2(110, 80));

    EXPECT_FLOAT_EQ(normalized.x, 0.5f);
    EXPECT_FLOAT_EQ(normalized.y, 0.5f);
    EXPECT_DOUBLE_EQ(viewport.aspectRatio(), 2.0);
}

}
