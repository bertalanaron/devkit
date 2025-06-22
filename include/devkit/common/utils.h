#pragma once
#include <tuple>
#include <unordered_map>
#include <string>
#include <optional>
#include <any>
#include <functional>
#include <variant>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <typeindex>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <future>
#include <semaphore>
#include <numeric>

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <magic_enum.hpp>

#include <devkit/common/constants.h>

#define DK_ASSERT assert

#define BIT(i) (1ull << i)

namespace details::common {

template <std::size_t ... Is, typename Tuple>
auto _reverse_tuple_impl(std::index_sequence<Is...>, Tuple&& tuple)
{
	return std::tuple<std::tuple_element_t<sizeof...(Is) - 1 - Is, std::decay_t<Tuple>>...>(std::get<sizeof...(Is) - 1 - Is>(tuple)...);
}

template <typename ... Ts>
auto _reverse_tuple(std::tuple<Ts...>&& tuple)
{
	return _reverse_tuple_impl(std::index_sequence_for<Ts...>(), tuple);
}

template<std::size_t... Is>
constexpr auto _reverse_index_sequence(std::index_sequence<Is...>) {
	return std::index_sequence<sizeof...(Is) - 1U - Is...>{};
}

template<typename... Ts, std::size_t... Is>
constexpr auto _reverse_args_impl(const std::tuple<Ts...>& t, std::index_sequence<Is...>) {
	return std::make_tuple(std::get<Is>(t)...);
}

} // details::common

namespace dk::common {

template <typename T, typename... Ts>
concept OfList = (std::same_as<T, Ts> || ...);

template <typename T>
struct id_t { using type = T; };

template <typename... Ts>
using reverse_tuple = decltype(details::common::_reverse_tuple(std::tuple<Ts...>()));

template<typename... Ts>
constexpr auto reverse_args(const Ts&... args) {
	auto original = std::forward_as_tuple(args...);
	constexpr auto N = sizeof...(Ts);
	return details::common::_reverse_args_impl(original, details::common::_reverse_index_sequence(std::make_index_sequence<N>{}));
}

template <typename... Ts> 
struct overload : Ts... { using Ts::operator()...; };

template <unsigned N>
struct string_literal {
	constexpr string_literal(const char(&str)[N]) {
		std::copy_n(str, N, value);
	}
	constexpr string_literal() { };
	char value[N]{};
};

template <unsigned N>
struct wstring_literal {
	constexpr wstring_literal(const wchar_t(&str)[N]) {
		std::copy_n(str, N, value);
	}
	constexpr wstring_literal() { };
	wchar_t value[N]{};
	const wchar_t* c_str() const { return value; }
};

template <typename D>
class SingletonBase {
public:
	SingletonBase() = default;
	SingletonBase(const SingletonBase&) = delete;
	SingletonBase& operator=(const SingletonBase&) = delete;

	static D& instance() {
		static D c_instance;
		return c_instance;
	}
};

template <typename T>
struct function_traits
	: public function_traits<decltype(&T::operator())>
{};

// Lambda, Callable or Method const
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
{
	enum { arity = sizeof...(Args) };

	typedef ReturnType result_type;

	template <size_t i>
	struct arg
	{
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};
};

// Lambda, Callable or Method mutable
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)>
{
	enum { arity = sizeof...(Args) };

	typedef ReturnType result_type;

	template <size_t i>
	struct arg
	{
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};
};

// Static member function, function pointer
template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)>
{
	enum { arity = sizeof...(Args) };

	typedef ReturnType result_type;

	template <size_t i>
	struct arg {
		static_assert(i < arity, "argument index out of range");
		using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
	};
};

template <typename T, size_t N, typename Fn>
std::array<T, N> fill_array(Fn&& generator)
{
	std::array<T, N> result;
	for (std::size_t i = 0; i < N; ++i) {
		result[i] = generator(i);
	}
	return result;
}

} // dk::common

namespace details::dbg {

template <typename T, dk::common::string_literal Label>
struct storage {
	inline static std::optional<T> value;
};

} // details::dbg

namespace dk::dbg {

template <typename T, common::string_literal Label>
	requires(std::is_default_constructible_v<T>)
T& store() {
	if (!details::dbg::storage<T, Label>::value.has_value())
		details::dbg::storage<T, Label>::value = T();
	return details::dbg::storage<T, Label>::value.value(); 
}

template <typename T, common::string_literal Label>
	requires(!std::is_default_constructible_v<T>)
T& store() {
	return details::dbg::storage<T, Label>::value.value(); 
}

template <typename T, common::string_literal Label>
const T& store_or(const T& fallback) {
	if (!details::dbg::storage<T, Label>::value.has_value())
		return fallback;
	return details::dbg::storage<T, Label>::value.value(); 
}

} // dk::dbg

namespace nlohmann {
template <typename Enum>
	requires std::is_enum_v<Enum>
void to_json(json& j, const Enum& e) {
	if (auto name = magic_enum::enum_name(e); !name.empty()) {
		j = std::string(name);
	} else {
		throw std::runtime_error("Invalid enum value");
	}
}

template <typename Enum>
	requires std::is_enum_v<Enum>
void from_json(const json& j, Enum& e) {
	if (auto val = magic_enum::enum_cast<Enum>(j.get<std::string>()); val.has_value()) {
		e = val.value();
	} else {
		throw std::runtime_error("Invalid enum name: " + j.get<std::string>());
	}
}
}

namespace YAML {
template <typename Enum>
	requires std::is_enum_v<Enum>
struct convert<Enum> {
	static Node encode(const Enum& e) {
		Node node;
		auto name = magic_enum::enum_name(e);
		if (!name.empty()) {
			node = std::string(name);
		}
		return node;
	}

	static bool decode(const Node& node, Enum& e) {
		if (!node.IsScalar())
			return false;

		auto val = magic_enum::enum_cast<Enum>(node.Scalar());
		if (val.has_value()) {
			e = val.value();
			return true;
		}
		return false;
	}
};
}

namespace glm {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec4, x, y, z, w);
}
