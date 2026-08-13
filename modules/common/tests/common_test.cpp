#include <devkit/common/utils.h>

#include <gtest/gtest.h>

namespace {

TEST(CommonTest, ReversesArgumentOrder)
{
    const auto values = dk::common::reverse_args(1, 2.5, std::string("three"));

    EXPECT_EQ(std::get<0>(values), "three");
    EXPECT_DOUBLE_EQ(std::get<1>(values), 2.5);
    EXPECT_EQ(std::get<2>(values), 1);
}

}
