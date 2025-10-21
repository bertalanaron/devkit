#pragma once
#include <devkit/common/utils.h>

namespace dk::io {

using button_t = uint8_t;
using modkey_t = uint8_t;
using key_t    = uint64_t;
using wheel_t  = uint8_t;

struct InputState {
	button_t buttons = 0;
	modkey_t modkeys = 0;
	key_t    keys    = 0;
	wheel_t  wheel   = 0;
	float    wheelDirection = 0;
};

InputState currentInputState();

InputState previousInputState();

#define DK_IO_DECL_IC(type, name, value) constexpr auto name = InputCombination(type##_mask::name);
#define DK_IO_DECL_MASK(name, value, ...) name = value,
#define DK_IO_SWITCH_CASE(type, name, value) case type##_mask::name: json = #name; break;
#define DK_IO_MAP_ENTRY(type, name, value) { #name, type##_mask::name },
#define DK_IO_MASK_JSON_CONVERSION(type, TABLE)                            \
	inline void to_json(nlohmann::json& json, const type##_mask& type) {   \
		switch (type) { TABLE(DK_IO_SWITCH_CASE, type) default: break; } } \
	inline void from_json(const nlohmann::json& json, type##_mask& type) { \
		static std::unordered_map<std::string, type##_mask> map = { TABLE(DK_IO_MAP_ENTRY, type) }; \
		if (auto it = map.find(std::string(json)); it != map.end())        \
			type = it->second; }                                           \
	/* end of macro */

#define DK_IO_BUTTONS_TABLE(F, ...)                \
	F( __VA_ARGS__ __VA_OPT__(,) left   , BIT(0) ) \
	F( __VA_ARGS__ __VA_OPT__(,) middle , BIT(1) ) \
	F( __VA_ARGS__ __VA_OPT__(,) right  , BIT(2) ) \
	F( __VA_ARGS__ __VA_OPT__(,) x1     , BIT(3) ) \
	F( __VA_ARGS__ __VA_OPT__(,) x2     , BIT(4) ) \
	/* end of macro */
#define DK_IO_MODKEYS_TABLE(F, ...)               \
	F( __VA_ARGS__ __VA_OPT__(,) none  , 0ll    ) \
	F( __VA_ARGS__ __VA_OPT__(,) shift , BIT(1) ) \
	F( __VA_ARGS__ __VA_OPT__(,) ctrl  , BIT(2) ) \
	F( __VA_ARGS__ __VA_OPT__(,) alt   , BIT(3) ) \
	F( __VA_ARGS__ __VA_OPT__(,) caps  , BIT(4) ) \
	/* end of macro */
#define DK_IO_KEYS_TABLE(F, ...)                       \
	F( __VA_ARGS__ __VA_OPT__(,)    _0     , BIT( 1) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _1     , BIT( 2) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _2     , BIT( 3) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _3     , BIT( 4) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _4     , BIT( 5) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _5     , BIT( 6) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _6     , BIT( 7) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _7     , BIT( 8) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _8     , BIT( 9) ) \
	F( __VA_ARGS__ __VA_OPT__(,)    _9     , BIT(10) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     a     , BIT(11) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     b     , BIT(12) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     c     , BIT(13) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     d     , BIT(14) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     e     , BIT(15) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     f     , BIT(16) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     g     , BIT(17) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     h     , BIT(18) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     i     , BIT(19) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     j     , BIT(20) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     k     , BIT(21) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     l     , BIT(22) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     m     , BIT(23) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     n     , BIT(24) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     o     , BIT(25) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     p     , BIT(26) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     q     , BIT(27) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     r     , BIT(28) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     s     , BIT(29) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     t     , BIT(30) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     u     , BIT(31) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     v     , BIT(32) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     w     , BIT(33) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     x     , BIT(34) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     y     , BIT(35) ) \
	F( __VA_ARGS__ __VA_OPT__(,)     z     , BIT(36) ) \
	F( __VA_ARGS__ __VA_OPT__(,) grave     , BIT(37) ) \
	F( __VA_ARGS__ __VA_OPT__(,)   esc     , BIT(38) ) \
	F( __VA_ARGS__ __VA_OPT__(,)   tab     , BIT(39) ) \
	F( __VA_ARGS__ __VA_OPT__(,)   del     , BIT(40) ) \
	F( __VA_ARGS__ __VA_OPT__(,)   win     , BIT(41) ) \
	F( __VA_ARGS__ __VA_OPT__(,) enter     , BIT(42) ) \
	F( __VA_ARGS__ __VA_OPT__(,) backspace , BIT(43) ) \
	F( __VA_ARGS__ __VA_OPT__(,) backslash , BIT(44) ) \
	/* end of macro */
#define DK_IO_WHEELS_TABLE(F, ...)               \
	F( __VA_ARGS__ __VA_OPT__(,) up   , BIT(1) ) \
	F( __VA_ARGS__ __VA_OPT__(,) down , BIT(2) ) \
	/* end of macro */

enum class button_mask : button_t { DK_IO_BUTTONS_TABLE(DK_IO_DECL_MASK) };
enum class modkey_mask : modkey_t { DK_IO_MODKEYS_TABLE(DK_IO_DECL_MASK) };
enum class key_mask    : key_t    { DK_IO_KEYS_TABLE(DK_IO_DECL_MASK)    };
enum class wheel_mask  : wheel_t  { DK_IO_WHEELS_TABLE(DK_IO_DECL_MASK)  };

DK_IO_MASK_JSON_CONVERSION(button, DK_IO_BUTTONS_TABLE)
DK_IO_MASK_JSON_CONVERSION(modkey, DK_IO_MODKEYS_TABLE)
DK_IO_MASK_JSON_CONVERSION(key   , DK_IO_KEYS_TABLE)
DK_IO_MASK_JSON_CONVERSION(wheel , DK_IO_WHEELS_TABLE)

class InputCombination {
public:
	constexpr InputCombination(button_mask button) : m_activator(button) { }
	constexpr InputCombination(modkey_mask modkey) : m_mods(modkey)      { }
	constexpr InputCombination(key_mask key)       : m_activator(key)    { }
	constexpr InputCombination(wheel_mask wheel)   : m_activator(wheel)  { }

	InputCombination operator+(const InputCombination& other) const;

	bool operator()(const InputState& state) const;

	explicit operator bool() const;

private:
	using Activator = std::variant<button_mask, key_mask, wheel_mask>;

	std::optional<modkey_mask> m_mods      = std::nullopt;
	std::optional<Activator>   m_activator = std::nullopt;

	friend void from_json(const nlohmann::json& json, InputCombination& ic);
	friend void to_json(nlohmann::json& json, const InputCombination& ic);
};

namespace button { DK_IO_BUTTONS_TABLE(DK_IO_DECL_IC, button) }
namespace modkey { DK_IO_MODKEYS_TABLE(DK_IO_DECL_IC, modkey) }
namespace key    { DK_IO_KEYS_TABLE(DK_IO_DECL_IC, key)       }
namespace wheel  { DK_IO_WHEELS_TABLE(DK_IO_DECL_IC, wheel)   }

class InputManager {
public:
	struct DefinitionMap {
		std::unordered_map<std::string, InputCombination> definitions;

		DefinitionMap() = default;
		DefinitionMap(const nlohmann::json& json);

		DefinitionMap& operator+=(const DefinitionMap& other);

		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(DefinitionMap, definitions);
	};

public:
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
	//NLOHMANN_DEFINE_TYPE_INTRUSIVE(InputManager, m_definitions);
};

}

namespace details::io {
void commitInputState(const dk::io::InputState& state);
bool imguiDoCaptureMouse();
void imguiDoCaptureMouse(bool enable);
bool imguiDoCaptureKeyboard();
void imguiDoCaptureKeyboard(bool enable);
}
