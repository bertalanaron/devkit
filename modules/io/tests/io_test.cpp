#include <devkit/io/input_combination.h>

#include <gtest/gtest.h>

namespace {

TEST(IoTest, InputCombinationMatchesActivatorAndModifier)
{
    const auto shortcut = dk::io::modkey::shift + dk::io::key::a;

    dk::io::InputState matching;
    matching.modkeys = static_cast<dk::io::modkey_t>(dk::io::modkey_mask::shift);
    matching.keys = static_cast<dk::io::key_t>(dk::io::key_mask::a);

    dk::io::InputState missingModifier = matching;
    missingModifier.modkeys = 0;

    EXPECT_TRUE(shortcut(matching));
    EXPECT_FALSE(shortcut(missingModifier));
}

}
