#include <devkit/io/input_combination.h>
#include <devkit/gfx/vertex.h>

#include <gtest/gtest.h>

namespace io_tests {

TEST(InputCombination, buttons) {
	EXPECT_EQ(dk::io::button::left(dk::io::InputCombination::makeState(0b00001)), true);
	EXPECT_EQ(dk::io::button::left(dk::io::InputCombination::makeState(0b00000)), false);
	EXPECT_EQ(dk::io::button::left(dk::io::InputCombination::makeState(0b01101)), true);
}

TEST(asfase, asfs) {
	constexpr dk::io::InputCombination ic1 = dk::io::modkey::ctrl + dk::io::key::s;
}

TEST(InputManager, define) {
	dk::io::InputManager inputManager;

	inputManager.define("save", dk::io::modkey::ctrl + dk::io::key::s);
	inputManager.define("cut" , dk::io::modkey::ctrl + dk::io::key::x);

	ASSERT_EQ(inputManager.isDefined("save"), true);
	ASSERT_EQ(inputManager.isDefined("cut") , true);
	ASSERT_EQ(inputManager.isDefined("load"), false);

	ASSERT_EQ(inputManager.inputCombinationOf("save"), dk::io::modkey::ctrl + dk::io::key::s);
	ASSERT_EQ(inputManager.inputCombinationOf("cut") , dk::io::modkey::ctrl + dk::io::key::x);
}

TEST(InputManager, evaluate) {
	dk::io::InputManager inputManager;
	inputManager.define("save", dk::io::modkey::ctrl + dk::io::key::s);
	inputManager.define("cut" , dk::io::modkey::ctrl + dk::io::key::x);

	// TODO: change inputManager state

	ASSERT_EQ(inputManager.active("save"), true);
	ASSERT_EQ(inputManager.active("cut"),  false);
}

TEST(InputManager, deserialization) {
	std::string source = 
	R"({
		"save": { "mod": ["ctrl"], "activator": { "type": "key", "value": "s" } },
		"cut":  { "mod": ["ctrl"], "activator": { "type": "key", "value": "x" } }
	})";
	nlohmann::json json(source);

	dk::io::InputManager inputManager;
	inputManager.define(json);

	ASSERT_EQ(inputManager.isDefined("save"), true);
	ASSERT_EQ(inputManager.isDefined("cut") , true);
	ASSERT_EQ(inputManager.isDefined("load"), false);
}

TEST(InputManager, serialize) {
	dk::io::InputManager inputManager;
	inputManager.define("save", dk::io::modkey::ctrl + dk::io::key::s);
	inputManager.define("cut" , dk::io::modkey::ctrl + dk::io::key::x);

	nlohmann::json serialized = inputManager;
	
	std::string source = 
		R"({
		"save": { "mod": ["ctrl"], "activator": { "type": "key", "value": "s" } },
		"cut":  { "mod": ["ctrl"], "activator": { "type": "key", "value": "x" } }
	})";
	nlohmann::json json(source);

	ASSERT_EQ(serialized, json);
}

}
