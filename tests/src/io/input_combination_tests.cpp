#include <devkit/io/input_combination.h>
#include <devkit/gfx/vertex.h>

#include <gtest/gtest.h>

namespace io_tests {

TEST(InputCombination, buttons) {
	EXPECT_EQ(dk::io::button::left(dk::io::InputCombination::makeState(0b00001)), true);
	EXPECT_EQ(dk::io::button::left(dk::io::InputCombination::makeState(0b00000)), false);
	EXPECT_EQ(dk::io::button::left(dk::io::InputCombination::makeState(0b01101)), true);
}

}
