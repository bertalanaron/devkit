#include <devkit/common/utils.h>

#include <gtest/gtest.h>

#include <rfl/json.hpp>
#include <rfl/yaml.hpp>

namespace {

TEST(CommonTest, ReversesArgumentOrder)
{
    const auto values = dk::common::reverse_args(1, 2.5, std::string("three"));

    EXPECT_EQ(std::get<0>(values), "three");
    EXPECT_DOUBLE_EQ(std::get<1>(values), 2.5);
    EXPECT_EQ(std::get<2>(values), 1);
}

TEST(CommonTest, ReflectsTemplatedGlmVectorsAndMatrices)
{
    const glm::dvec2 vector{1.25, -2.5};
    const auto vector_yaml = rfl::yaml::write(vector);
    const auto parsed_vector = rfl::yaml::read<glm::dvec2>(vector_yaml);

    ASSERT_TRUE(parsed_vector.has_value());
    EXPECT_DOUBLE_EQ(parsed_vector->x, vector.x);
    EXPECT_DOUBLE_EQ(parsed_vector->y, vector.y);

    const glm::vec3 vector3{1.0f, 2.0f, 3.0f};
    const auto parsed_vector3 = rfl::json::read<glm::vec3>(rfl::json::write(vector3));
    ASSERT_TRUE(parsed_vector3.has_value());
    EXPECT_EQ(*parsed_vector3, vector3);

    const glm::dvec4 vector4{4.0, 5.0, 6.0, 7.0};
    const auto parsed_vector4 = rfl::yaml::read<glm::dvec4>(rfl::yaml::write(vector4));
    ASSERT_TRUE(parsed_vector4.has_value());
    EXPECT_EQ(*parsed_vector4, vector4);

    const glm::dmat3 matrix{
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.0,
    };
    const auto matrix_json = rfl::json::write(matrix);
    const auto parsed_matrix = rfl::json::read<glm::dmat3>(matrix_json);

    ASSERT_TRUE(parsed_matrix.has_value());
    for (glm::length_t column = 0; column < 3; ++column)
        for (glm::length_t row = 0; row < 3; ++row)
            EXPECT_DOUBLE_EQ((*parsed_matrix)[column][row], matrix[column][row]);

    const glm::mat4 matrix4{1.0f};
    const auto parsed_matrix4 = rfl::yaml::read<glm::mat4>(rfl::yaml::write(matrix4));
    ASSERT_TRUE(parsed_matrix4.has_value());
    for (glm::length_t column = 0; column < 4; ++column)
        for (glm::length_t row = 0; row < 4; ++row)
            EXPECT_FLOAT_EQ((*parsed_matrix4)[column][row], matrix4[column][row]);
}

}
