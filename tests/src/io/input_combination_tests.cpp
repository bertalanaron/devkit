//#include <devkit/io/input_combination.h>
#include <devkit/gfx/vertex.h>

#include <gtest/gtest.h>

namespace dk::io {

using button_t = uint8_t;
using modkey_t = uint8_t;
using key_t    = uint64_t;
using wheel_t  = uint8_t;

struct InputState {
	button_t buttons;
	modkey_t modkeys;
	key_t    keys;
	wheel_t  wheel;
	float    wheelDirection;
};

InputState currentInputState();

InputState previousInputState();

class InputCombination {
public:
	using Activator = std::variant<button_t, key_t, wheel_t>;

	constexpr InputCombination(button_t) { }
	constexpr InputCombination(modkey_t) { }
	constexpr InputCombination(key_t) { }

	constexpr InputCombination operator+(const InputCombination& other) const;

	bool operator()(const InputState& state) const;

private:
	std::optional<std::vector<modkey_t>> m_mods;
	std::optional<Activator>             m_activator;

	friend void from_json(const nlohmann::json& json, InputCombination& ic);
	friend void to_json(nlohmann::json& json, const InputCombination& ic);
};

#define DK_IO_DECL_IC(type, name, index) constexpr auto name = InputCombination((type##_t)type##_mask::name);
#define DK_IO_DECL_MASK(name, index, ...) name = BIT(index),
#define DK_IO_BUTTONS_TABLE(F, ...)           \
	F( __VA_ARGS__ __VA_OPT__(,) left   , 1 ) \
	F( __VA_ARGS__ __VA_OPT__(,) middle , 2 ) \
	F( __VA_ARGS__ __VA_OPT__(,) right  , 3 ) \
	F( __VA_ARGS__ __VA_OPT__(,) x1     , 4 ) \
	F( __VA_ARGS__ __VA_OPT__(,) x2     , 5 ) \
	/* end of macro */
#define DK_IO_MODKEYS_TABLE(F, ...)          \
	F( __VA_ARGS__ __VA_OPT__(,) shift , 1 ) \
	F( __VA_ARGS__ __VA_OPT__(,) ctrl  , 2 ) \
	F( __VA_ARGS__ __VA_OPT__(,) alt   , 3 ) \
	F( __VA_ARGS__ __VA_OPT__(,) caps  , 4 ) \
	/* end of macro */
#define DK_IO_KEYS_TABLE(F, ...)             \
	F( __VA_ARGS__ __VA_OPT__(,)    _0,  1 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _1,  2 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _2,  3 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _3,  4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _4,  5 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _5,  6 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _6,  7 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _7,  8 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _8,  9 ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _9, 10 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     a, 11 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     b, 12 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     c, 13 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     d, 14 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     e, 15 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     f, 16 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     g, 17 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     h, 18 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     h, 19 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     j, 20 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     k, 21 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     l, 22 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     m, 23 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     n, 24 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     o, 25 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     p, 26 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     q, 27 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     r, 28 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     s, 29 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     t, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     u, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     v, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     w, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     x, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     y, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)     z, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,) tilda, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)   esc, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)   tab, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)   del, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,)   win, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,) enter, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,) backspace, 30 ) \
	F( __VA_ARGS__ __VA_OPT__(,) backslash, 30 ) \
	/* end of macro */

enum class button_mask : button_t { DK_IO_BUTTONS_TABLE(DK_IO_DECL_MASK) };
namespace button { DK_IO_BUTTONS_TABLE(DK_IO_DECL_IC, button) }

enum class modkey_mask : modkey_t { DK_IO_MODKEYS_TABLE(DK_IO_DECL_MASK) };
namespace modkey { DK_IO_MODKEYS_TABLE(DK_IO_DECL_IC, modkey) }

enum class key_mask : key_t { DK_IO_KEYS_TABLE(DK_IO_DECL_MASK) };
namespace key { DK_IO_KEYS_TABLE(DK_IO_DECL_IC, key) }

class InputManager {
public:
	struct DefinitionMap {
		std::unordered_map<std::string, InputCombination> definitions;

		DefinitionMap(const nlohmann::json& json);

		DefinitionMap& operator+=(DefinitionMap&& other);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(DefinitionMap, definitions);
	};

public:
	InputManager();

	void define(const std::string& name, const InputCombination& inputCombination);

	void define(DefinitionMap&& definitions);

	bool isDefined(const std::string& name) const;

	const InputCombination& inputCombinationOf(const std::string& name) const;

	// @brief Returns true if a definition with the given name exists and it's input combination is active
	bool active(const std::string& name) const
	{
		auto it = m_definitions.definitions.find(name);
		if (it == m_definitions.definitions.end())
			return false;
		return it->second(currentInputState());
	}

	// @brief Returns true if a definition with the given name exists 
	// and it's input combination became active in the last tick
	bool activated(const std::string& name) const
	{
		auto it = m_definitions.definitions.find(name);
		if (it == m_definitions.definitions.end())
			return false;
		return it->second(currentInputState()) && !it->second(previousInputState());
	}

	// @brief Returns true if a definition with the given name exists 
	// and it's input combination became inactive in the last tick
	bool deactivated(const std::string& name) const 
	{
		auto it = m_definitions.definitions.find(name);
		if (it == m_definitions.definitions.end())
			return false;
		return !it->second(currentInputState()) && it->second(previousInputState());
	}

private:
	DefinitionMap m_definitions;

public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(InputManager, m_definitions);
};


}

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
