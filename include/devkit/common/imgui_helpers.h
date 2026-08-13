#pragma once
#include <devkit/common/utils.h>
#include <devkit/io/asset_manager.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/texture.h>

#include <imgui.h>

namespace dk::imgui_helpers {

// ======================== SCALARS ========================

template <typename T>
struct scalar_editor
{
	const T min  = std::numeric_limits<T>::lowest();
	const T max  = std::numeric_limits<T>::max();
	const T step = T(1);
};

template <int N = 1, scalar_editor<float> Editor = scalar_editor<float>{}>
inline void edit(const char* id, float& value)
{ ImGui::DragScalarN(id, ImGuiDataType_Float, &value, N, Editor.step, &Editor.min, &Editor.max); }

template <int N = 1, scalar_editor<double> Editor = scalar_editor<double>{}>
inline void edit(const char* id, double& value)
{ ImGui::DragScalarN(id, ImGuiDataType_Double, &value, N, Editor.step, &Editor.min, &Editor.max); }

template <int N = 1, scalar_editor<int> Editor = scalar_editor<int>{}>
inline void edit(const char* id, int& value)
{ ImGui::DragScalarN(id, ImGuiDataType_S32, &value, N, Editor.step, &Editor.min, &Editor.max); }

// ======================== STRING =========================

struct string_editor {
	const size_t capacity = 256ull;
};

template <string_editor Editor = string_editor{}>
inline void edit(const char* id, std::string& value)
{
	char buffer[Editor.capacity];
	std::snprintf(buffer, Editor.capacity, "%s", value.c_str());
	if (ImGui::InputText(id, buffer, Editor.capacity))
		value = buffer;
}

// ========================== BOOL =========================

inline void edit(const char* id, bool& value)
{ ImGui::Checkbox(id, &value); }

// ========================= ENUMS =========================

template <typename E>
	requires(std::is_enum_v<E>)
inline void edit(const char* id, E& value)
{
	const auto names = magic_enum::enum_names<E>();
	const int index = magic_enum::enum_index(value).value_or(0);

	if (ImGui::BeginCombo(id, names[index].data())) {
		for (int i = 0; i < static_cast<int>(names.size()); ++i) {
			bool selected = (i == index);
			if (ImGui::Selectable(names[i].data(), selected)) {
				value = magic_enum::enum_value<E>(i);
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}

template <typename E>
	requires(std::is_enum_v<E>)
inline E edit(const char* id, const E& value)
{
	E result = value;
	const auto names = magic_enum::enum_names<E>();
	const int index = magic_enum::enum_index(value).value_or(0);

	if (ImGui::BeginCombo(id, names[index].data())) {
		for (int i = 0; i < static_cast<int>(names.size()); ++i) {
			bool selected = (i == index);
			if (ImGui::Selectable(names[i].data(), selected)) {
				result = magic_enum::enum_value<E>(i);
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return result;
}

// ======================= VECTORS =========================

namespace details {

template <typename T>
struct is_glm_vec : std::false_type {};

template <glm::length_t L, typename T, glm::qualifier Q>
struct is_glm_vec<glm::vec<L, T, Q>> : std::true_type {};

template <typename T>
concept glm_vec = is_glm_vec<T>::value;

}

template <glm::length_t L, typename T, glm::qualifier Q>
inline void edit(const char* id, glm::vec<L, T, Q>& vec)
{ edit<L>(id, vec.x); }

// ====================== PROPERTIES =======================

template <typename UProperty>
constexpr scalar_editor<typename UProperty::type> scalar_config_editor = {};

#define DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(Type, min, max, step)           \
	template <>                                                              \
	constexpr scalar_editor<typename Type::type> scalar_config_editor<Type> = { min, max, step };\
	/* end of macro */

template <common::UniquePropertySpecialization UProperty>
	requires(std::is_scalar_v<typename UProperty::type> && !std::is_same_v<typename UProperty::type, bool>)
typename UProperty::type edit(const char* name, const UProperty& uprop)
{
	constexpr auto editor = scalar_config_editor<UProperty>;
	typename UProperty::type result = uprop.value;
	edit<1, editor>(name, result);
	return result;
}

template <common::UniquePropertySpecialization UProperty>
constexpr string_editor string_config_editor = {};

#define DK_IMHELPER_STRING_CONFIG_CAPACITY(Type, capacity)             \
	template <>                                                        \
	constexpr string_editor string_config_editor<Type> = { capacity }; \
	/* end of macro */

template <common::UniquePropertySpecialization UProperty>
	requires(std::is_same_v<typename UProperty::type, std::string>)
inline typename UProperty::type edit(const char* name, const UProperty& uprop)
{
	typename UProperty::type result = uprop.value;
	constexpr static auto editor = string_config_editor<UProperty>;
	edit<editor>(name, result);
	return result;
}

template <common::UniquePropertySpecialization UProperty>
	requires(std::is_same_v<typename UProperty::type, bool>)
inline typename UProperty::type edit(const char* name, const UProperty& uprop)
{
	typename UProperty::type result = uprop.value;
	edit(name, result);
	return result;
}

template <common::UniquePropertySpecialization UProperty>
	requires(details::glm_vec<typename UProperty::type>)
inline typename UProperty::type edit(const char* name, const UProperty& uprop)
{ 
	typename UProperty::type result = uprop.value;
	edit(name, result); 
	return result;
}

template <common::ConfigurationSpecialization Config>
inline void edit(const char* name, Config& config)
{
	if (ImGui::BeginTable(name, 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Label"  , ImGuiTableColumnFlags_WidthFixed/*, labelColumnWidth*/);
		ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

		config.for_each([name,&config](const auto& prop) {
			const std::string_view prop_name = Config::property_name(prop);
			const std::string label = std::format("{}.{}", name, prop_name);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text(prop_name.data());
			ImGui::TableSetColumnIndex(1);
			config.template set<std::decay_t<decltype(prop)>>(edit(label.c_str(), prop));
		});

		ImGui::EndTable();
	}
}

// ==================== DRAW IMAGE =======================

inline void draw(gfx::Texture2D& tex)
{
	// Get the window size
	ImVec2 windowSize = ImGui::GetWindowSize();

	// Calculate aspect ratio of the texture
	float textureAspect = tex.size().x / tex.size().y;

	// Calculate the scaling factor to maintain aspect ratio
	float windowAspect = windowSize.x / windowSize.y;
	float scaleX, scaleY;

	if (windowAspect > textureAspect) {
		// Scale based on the window's height
		scaleY = windowSize.y;
		scaleX = scaleY * textureAspect;
	} else {
		// Scale based on the window's width
		scaleX = windowSize.x;
		scaleY = scaleX / textureAspect;
	}

	// Calculate the position to center the image
	ImVec2 pos = ImGui::GetCursorScreenPos();
	float offsetX = (windowSize.x - scaleX) * 0.5f;
	float offsetY = (windowSize.y - scaleY) * 0.5f;

	// Apply the offset to the position to center the image
	pos.x += offsetX;
	pos.y += offsetY;

	// Now, render the image with the calculated size and position
	ImGui::GetWindowDrawList()->AddImage(
		(ImTextureID)(intptr_t)tex.handle(), pos,
		ImVec2(pos.x + scaleX, pos.y + scaleY)
	);
}

} // namespace dk::imgui_helpers

#include <devkit/io/window.h>

// namespace dk::imgui_helpers {
// DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(io::Window::Opacity, 0, 1, 0.01);
// }
