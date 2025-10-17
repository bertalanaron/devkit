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
#include <expected>
#include <deque>
#include <queue>
#include <numbers>
#include <bitset>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <magic_enum/magic_enum.hpp>

#include <devkit/common/constants.h>

#define DK_ASSERT assert

#define BIT(i) (1ull << i)

#ifndef MAGIC_ENUM_ENABLE_ENUM_FLAGS
#define MAGIC_ENUM_ENABLE_ENUM_FLAGS(type)         \
template <>                                        \
struct magic_enum::customize::enum_range<type> { \
	static constexpr bool is_flags = true;         \
};
#endif

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

template <typename T, typename... Ts>
constexpr bool is_of_list = false;

template <typename T, typename... Ts>
	requires OfList<T, Ts...>
constexpr bool is_of_list<T, Ts...> = true;

template <typename T>
struct id_t { using type = T; };

template <typename T>
constexpr auto id = id_t<T>{};

template <typename, template <typename...> typename>
constexpr bool is_specialization_of = false;

template <template <typename...> typename Base, typename... Args>
constexpr bool is_specialization_of<Base<Args...>, Base> = true;

template <typename...>
constexpr bool is_each_unique = true;

template <typename T, typename... Rest>
constexpr bool is_each_unique<T, Rest...> =
(!std::is_same_v<T, Rest> && ...) && is_each_unique<Rest...>;

template <typename... Ts>
constexpr bool is_each_unique<std::tuple<Ts...>> = is_each_unique<Ts...>;

template <typename... Ts>
using reverse_tuple = decltype(details::common::_reverse_tuple(std::tuple<Ts...>()));

template<typename... Ts>
constexpr auto reverse_args(const Ts&... args) {
	auto original = std::forward_as_tuple(args...);
	constexpr auto N = sizeof...(Ts);
	return details::common::_reverse_args_impl(original, details::common::_reverse_index_sequence(std::make_index_sequence<N>{}));
}

template <typename Tuple, typename F>
constexpr void for_each_in_tuple(Tuple&& t, F&& f)
{
	std::apply([&f](auto&&... elems) { (f(std::forward<decltype(elems)>(elems)), ...); },
		std::forward<decltype(t)>(t));
}

template <typename T, typename... Ts>
	requires (is_each_unique<Ts...>)
constexpr std::size_t index_of =
	[]<std::size_t... Is>(std::index_sequence<Is...>) {
		constexpr bool matches[] = { std::is_same_v<T, Ts>... };
		for (std::size_t i = 0; i < sizeof...(Ts); ++i)
			if (matches[i]) return i;
		return static_cast<std::size_t>(-1);
	}(std::index_sequence_for<Ts...>{});

template <typename E>
	requires(std::is_enum_v<E>)
E toggle(const E& e)
{ return E(!(bool)e); }

template <typename... Ts> 
struct overload : Ts... { using Ts::operator()...; };

template <unsigned N>
struct string_literal {
	constexpr string_literal(const char(&str)[N]) {
		std::copy_n(str, N, value);
	}
	constexpr string_literal() { };
	char value[N]{};

	operator std::string_view() const { return value; }
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

template <typename T>
struct copyable_unique_ptr {
	using pointer = std::unique_ptr<T>::pointer;
	std::unique_ptr<T> ptr;

	copyable_unique_ptr(std::unique_ptr<T>&& p) : ptr(std::move(p)) {}
	copyable_unique_ptr(const copyable_unique_ptr& other)
		: ptr(other.ptr ? std::make_unique<T>(*other.ptr) : nullptr) {}
	copyable_unique_ptr& operator=(const copyable_unique_ptr& other) {
		if (this != &other)
			ptr = other.ptr ? std::make_unique<T>(*other.ptr) : nullptr;
		return *this;
	}
	copyable_unique_ptr(copyable_unique_ptr&&) noexcept = default;
	copyable_unique_ptr& operator=(copyable_unique_ptr&&) noexcept = default;

	constexpr pointer get() const 
	{ return ptr.get(); }

	constexpr pointer operator->() const
	{ return ptr.operator->(); }

	constexpr std::add_lvalue_reference_t<T> operator*() const 
	{ return *ptr; }

	operator bool() const 
	{ return (bool)ptr; }
};

template <typename T>
auto make_copyable_unique(auto&&... args)
{ return copyable_unique_ptr(std::make_unique<T>(args...)); }

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

class ScopeGuard
{
public:
	ScopeGuard(const auto& engageFunc, const auto& releaseFunc)
		: m_releaseFunc(releaseFunc)
	{
		engageFunc();
		m_engaged = true;
	}

	ScopeGuard(const auto& releaseFunc)
		: m_engaged(true)
		, m_releaseFunc(releaseFunc)
	{ }

	// @brief Stop release callback from executing
	void release() { m_engaged = false; }

	~ScopeGuard()
	{
		if (m_engaged)
			m_releaseFunc();
	}

private:
	bool                  m_engaged = false;
	std::function<void()> m_releaseFunc;
};


template <typename Base>
class watched_object : private Base {
public:
	using Base::Base;

	const Base& get() const { return *static_cast<const Base*>(this); }

	Base& modify()
	{
		m_dirty = true;
		return *static_cast<Base*>(this);	
	}

	bool dirty() const { return m_dirty; }
	void reset_dirty_flag() { m_dirty = false; }

private:
	bool m_dirty = true;
};

template <class T>
struct factory_method
{
	T operator()(auto&&... args) const
	{ return T(std::forward<decltype(args)>(args)...); }
};

template <class T>
factory_method<T> make_factory()
{ return {}; }

template <typename Derived, template <typename...> class BaseTemplate>
struct is_specialization_or_derived {
private:
	// helper that succeeds if Derived is convertible to BaseTemplate<Args...>*
	template <typename... Args>
	static std::true_type test(BaseTemplate<Args...>*);

	// fallback
	static std::false_type test(...);

public:
	static constexpr bool value = decltype(test(std::declval<Derived*>()))::value;
};

template <typename Derived, template <typename...> class BaseTemplate>
constexpr bool is_specialization_or_derived_v = is_specialization_or_derived<Derived, BaseTemplate>::value;

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

std::filesystem::path executable_path();

namespace fs {

bool is_parent(const std::filesystem::path& path, const std::filesystem::path& parent);

bool is_direct_child(const std::filesystem::path& path, const std::filesystem::path& child);

}

} // dk::common

namespace magic_enum_extension {

template <typename E>
std::vector<std::string> enum_flags(E flags) 
{
	std::vector<std::string> result;
	using T = std::underlying_type_t<E>;

	for (const auto& entry : magic_enum::enum_entries<E>())
	{
		if (!(static_cast<T>(flags) & static_cast<T>(entry.first)))
			continue;
		result.emplace_back(entry.second);
	}
	return result;
}

}

namespace nlohmann_extension {

void smart_dump(const nlohmann::json& j, std::ostream& os, int indent = 0, int indent_step = 2, int threshold = 80);

std::string smart_dump(const nlohmann::json& j, int indent = 0, int indent_step = 2, int threshold = 80);

}

namespace details::dbg {

template <typename T, dk::common::string_literal Label>
struct storage {
	inline static std::optional<T> value;
};

template <typename T, dk::common::string_literal Label>
struct threadlocal_storage {
	inline static std::unordered_map<std::thread::id, T> values;
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

template <typename T, common::string_literal Label>
	requires(std::is_default_constructible_v<T>)
T& store_threadlocal()
{
	return details::dbg::threadlocal_storage<T, Label>::values[std::this_thread::get_id()];
}

template <typename T, common::string_literal Label>
const T& store_threadlocal_or(const T& fallback)
{
	auto it = details::dbg::threadlocal_storage<T, Label>::values.find(std::this_thread::get_id());
	if (it == details::dbg::threadlocal_storage<T, Label>::values.end())
		return fallback;
	return it->second;
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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( vec2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ivec2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(uvec2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(dvec2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( vec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ivec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(uvec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(dvec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( vec4, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ivec4, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(uvec4, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(dvec4, x, y, z, w);
}

template <>
struct std::hash<glm::dvec2> {
	std::size_t operator()(const glm::dvec2& v) const {
		std::size_t h1 = std::hash<glm::dvec2::value_type>{}(v.x);
		std::size_t h2 = std::hash<glm::dvec2::value_type>{}(v.y);
		return h1 ^ (h2 << 1);
	}
};

template <>
struct std::hash<glm::ivec2> {
	std::size_t operator()(const glm::ivec2& v) const {
		std::size_t h1 = std::hash<glm::ivec2::value_type>{}(v.x);
		std::size_t h2 = std::hash<glm::ivec2::value_type>{}(v.y);
		return h1 ^ (h2 << 1);
	}
};

#define GLM_FMT(type, formatstring, ...)						     \
	template <>                                                      \
	struct fmt::formatter<glm::type> {                               \
		constexpr auto parse(format_parse_context& ctx)              \
		{ return ctx.begin(); }                                      \
		template <typename FormatContext>                            \
		auto format(const glm::type& v, FormatContext& ctx) const {  \
			return format_to(ctx.out(), formatstring __VA_OPT__(,) __VA_ARGS__ ); \
		}                                                            \
	};                                                               \
	/* end of macro */

GLM_FMT(vec2 , "({}, {})", v.x, v.y)
GLM_FMT(ivec2, "({}, {})", v.x, v.y)
GLM_FMT(uvec2, "({}, {})", v.x, v.y)
GLM_FMT(dvec2, "({}, {})", v.x, v.y)

GLM_FMT(vec3 , "({}, {}, {})", v.x, v.y, v.z)
GLM_FMT(ivec3, "({}, {}, {})", v.x, v.y, v.z)
GLM_FMT(uvec3, "({}, {}, {})", v.x, v.y, v.z)
GLM_FMT(dvec3, "({}, {}, {})", v.x, v.y, v.z)

GLM_FMT(vec4 , "({}, {}, {})", v.x, v.y, v.z, v.w)
GLM_FMT(ivec4, "({}, {}, {})", v.x, v.y, v.z, v.w)
GLM_FMT(uvec4, "({}, {}, {})", v.x, v.y, v.z, v.w)
GLM_FMT(dvec4, "({}, {}, {})", v.x, v.y, v.z, v.w)

#define __DK_WRAP(...) (__VA_ARGS__),
#define __DK_EXPAND(X) X
#define __DK_EXPAND_ALL(...) __VA_ARGS__

#define __DK_CONCAT2(A, B) A##B
#define __DK_CONCAT2_DEFERRED(A, B) __DK_CONCAT2(A, B)
#define __DK_CONCAT3(A, B, C) A##B##C
#define __DK_CONCAT3_DEFERRED(A, B, C) __DK_CONCAT3(A, B, C)

#define __DK_IF_0(trueCase, falseCase) falseCase
#define __DK_IF_1(trueCase, falseCase) trueCase
#define __DK_IF(condition, trueCase, falseCase) \
	__DK_CONCAT2_DEFERRED(__DK_IF_, condition)(trueCase, falseCase)

#define __DK_OPT_0(x) 
#define __DK_OPT_1(x) x
#define DK_OPT(check, x) __DK_CONCAT2_DEFERRED(__DK_OPT_, check)(x)

#define __DK_OPT_COMMA_0
#define __DK_OPT_COMMA_1 ,
// Places a comma if check is 1 and nothing if check is 0
#define DK_OPT_COMMA(check) __DK_CONCAT2_DEFERRED(__DK_OPT_COMMA_, check)

#define __DK_TABLE_AT_0( X, ...) X 
#define __DK_TABLE_AT_1( _0, X, ...) X 
#define __DK_TABLE_AT_2( _0, _1, X, ...) X 
#define __DK_TABLE_AT_3( _0, _1, _2, X, ...) X 
#define __DK_TABLE_AT_4( _0, _1, _2, _3, X, ...) X 
#define __DK_TABLE_AT_5( _0, _1, _2, _3, _4, X, ...) X 
#define __DK_TABLE_AT_6( _0, _1, _2, _3, _4, _5, X, ...) X 
#define __DK_TABLE_AT_7( _0, _1, _2, _3, _4, _5, _6, X, ...) X 
#define __DK_TABLE_AT_8( _0, _1, _2, _3, _4, _5, _6, _7, X, ...) X 
#define __DK_TABLE_AT_9( _0, _1, _2, _3, _4, _5, _6, _7, _8, X, ...) X 
#define __DK_TABLE_AT_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, X, ...) X 
#define __DK_TABLE_AT_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, X, ...) X 
#define __DK_TABLE_AT_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, X, ...) X 
#define __DK_TABLE_AT_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, X, ...) X 
#define __DK_TABLE_AT_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, X, ...) X 
#define __DK_TABLE_AT_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, X, ...) X 
#define __DK_TABLE_AT_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, X, ...) X 
#define __DK_TABLE_AT_17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, X, ...) X 
#define __DK_TABLE_AT_18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, X, ...) X 
#define __DK_TABLE_AT_19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, X, ...) X 
#define __DK_TABLE_AT_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, X, ...) X 
#define __DK_TABLE_AT_INDIRECT(IDX, ...) __DK_TABLE_AT_##IDX##(__VA_ARGS__)
#define DK_TABLE_AT(IDX, TABLE) __DK_TABLE_AT_INDIRECT(IDX, TABLE(__DK_WRAP))
#define DK_EXPAND_TABLE_AT(IDX, TABLE) __DK_CONCAT2_DEFERRED(__DK_EXPAND_ALL, DK_TABLE_AT(IDX, TABLE))

#define __DK_MEMBERS_XY(F  , ...) F(__VA_ARGS__ __VA_OPT__(,) x , 1) F(__VA_ARGS__ __VA_OPT__(,) y , 0)
#define __DK_MEMBERS_XYZ(F , ...) F(__VA_ARGS__ __VA_OPT__(,) x , 1) F(__VA_ARGS__ __VA_OPT__(,) y , 1) F(__VA_ARGS__ __VA_OPT__(,) z , 0)
#define __DK_MEMBERS_XYZW(F, ...) F(__VA_ARGS__ __VA_OPT__(,) x , 1) F(__VA_ARGS__ __VA_OPT__(,) y , 1) F(__VA_ARGS__ __VA_OPT__(,) z , 1) F(__VA_ARGS__ __VA_OPT__(,) w , 0)
#define __DK_MEMBERS_RGBA(F, ...) F(__VA_ARGS__ __VA_OPT__(,) r , 1) F(__VA_ARGS__ __VA_OPT__(,) g , 1) F(__VA_ARGS__ __VA_OPT__(,) b , 1) F(__VA_ARGS__ __VA_OPT__(,) a , 0)

#define __DK_JOIN_WITH_LUT_GETIDX(IDX, ...) IDX
#define __DK_JOIN_WITH_LUT_ELEM(F, LUT, IDX, ...) F(IDX, __VA_ARGS__, DK_EXPAND_TABLE_AT(IDX, LUT))
// Requires the index to be TABLE's first element
#define __DK_JOIN_WITH_LUT(LUT, TABLE, F) TABLE(__DK_JOIN_WITH_LUT_ELEM, F, LUT)

#define __DK_STR(X) #X
