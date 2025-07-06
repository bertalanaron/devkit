#include <devkit/io/input_combination.h>

dk::io::InputState g_currentInputState;
dk::io::InputState g_previousInputState;

bool g_imguiDoCaptureMouse    = true;
bool g_imguiDoCaptureKeyboard = true;

void details::io::commitInputState(const dk::io::InputState& state)
{
	g_previousInputState = g_currentInputState;
	g_currentInputState = state;
}

bool details::io::imguiDoCaptureMouse()
{
	return g_imguiDoCaptureMouse;
}

void details::io::imguiDoCaptureMouse(bool enable)
{
	g_imguiDoCaptureMouse = enable;
}

bool details::io::imguiDoCaptureKeyboard() 
{ 
	return g_imguiDoCaptureKeyboard; 
}

void details::io::imguiDoCaptureKeyboard(bool enable)
{
	g_imguiDoCaptureKeyboard = enable; 
}

dk::io::InputState dk::io::currentInputState()
{
	return g_currentInputState;
}

dk::io::InputState dk::io::previousInputState()
{
	return g_previousInputState;
}

void dk::io::from_json(const nlohmann::json& json, InputCombination& ic)
{
	if (json.contains("button")) ic.m_activator = [&] { button_mask mask = json["button"]; return mask; }();
	if (json.contains("key"   )) ic.m_activator = [&] { key_mask    mask = json["key"]   ; return mask; }();
	if (json.contains("wheel" )) ic.m_activator = [&] { wheel_mask  mask = json["wheel"] ; return mask; }();

	if (json.contains("mod")) {
		ic.m_mods = (modkey_mask)0;
		for (const auto& mod : json["mods"].array())
			ic.m_mods.value() = modkey_mask((modkey_t)ic.m_mods.value() | modkey_t(mod));
	}
}

void dk::io::to_json(nlohmann::json& json, const InputCombination& ic)
{
	if (ic.m_activator.has_value()) {
		std::visit(common::overload {
			[&](button_mask button) { json["button"] = nlohmann::json(button); },
			[&](key_mask key) { json["key"] = nlohmann::json(key); },
			[&](wheel_mask wheel) { json["wheel"] = nlohmann::json(wheel); }
			}, ic.m_activator.value());
	}
	if (ic.m_mods.has_value())
		json["mod"] = magic_enum_extension::enum_flags(ic.m_mods.value());
}

dk::io::InputCombination dk::io::InputCombination::operator+(const InputCombination& other) const
{
	if (m_activator.has_value() && other.m_activator.has_value())
		throw std::runtime_error("An inputcombination may only have at most one activator.");

	InputCombination newIc = *this;

	if (m_activator.has_value())
		newIc.m_activator = m_activator;
	if (other.m_activator.has_value())
		newIc.m_activator = other.m_activator;

	if (m_mods.has_value() || other.m_mods.has_value()) {
		newIc.m_mods = (modkey_mask)0;
		
		*(modkey_t*)&newIc.m_mods.value() |= (modkey_t)m_mods.value_or(modkey_mask(0));
		*(modkey_t*)&newIc.m_mods.value() |= (modkey_t)other.m_mods.value_or(modkey_mask(0));
	}
	
	return newIc;
}

bool dk::io::InputCombination::operator()(const InputState& state) const
{
	bool result = m_activator.has_value() || m_mods.has_value();
	if (m_activator.has_value()) {
		std::visit(common::overload{
			[&](const button_mask& button) { if (!((button_t)state.buttons & (button_t)button)) result = false; },
			[&](const key_mask&    key)    { if (!(   (key_t)state.keys    &    (key_t)key   )) result = false; },
			[&](const wheel_mask&  wheel)  { if (!( (wheel_t)state.wheel   &  (wheel_t)wheel )) result = false; }
			}, m_activator.value());
	}

	if (m_mods.has_value()) 
		if (((modkey_t)state.modkeys & (modkey_t)m_mods.value()) != (modkey_t)m_mods.value()) result = false;
	if (m_mods.has_value() && m_mods.value() == modkey_mask::none)
		if (state.modkeys) result = false;

	return result;
}

dk::io::InputCombination::operator bool() const
{
	return (*this)(currentInputState());
}

void dk::io::InputManager::define(const std::string& name, const InputCombination& inputCombination)
{
	m_definitions.definitions.insert({ name, inputCombination });
}

void dk::io::InputManager::define(DefinitionMap&& definitions)
{
	m_definitions += std::move(definitions);
}

bool dk::io::InputManager::isDefined(const std::string& name) const
{
	return m_definitions.definitions.contains(name);
}

const dk::io::InputCombination& dk::io::InputManager::inputCombinationOf(const std::string& name) const
{
	return m_definitions.definitions.find(name)->second;
}

dk::io::InputManager::DefinitionMap& dk::io::InputManager::DefinitionMap::operator+=(const DefinitionMap& other)
{
	definitions.insert(other.definitions.begin(), other.definitions.end());
	return *this;
}
