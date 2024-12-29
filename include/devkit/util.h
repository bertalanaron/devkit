#pragma once
#include <tuple>
#include <utility>
#include <optional>
#include <variant>

#define NS_DEVKIT dk

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>
#undef DELETE

namespace glm {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec2, x, y)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec3, x, y, z)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec4, x, y, z, w)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::i32vec2, x, y)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::u32vec2, x, y)

}

namespace nlohmann {

template <typename T>
struct adl_serializer<std::optional<T>> {
	static void to_json(json& j, const std::optional<T>& opt) {
		if (opt.has_value())
			j = *opt;
		else
			j = nullptr;
	}

	static void from_json(const json& j, std::optional<T>& opt) {
		if (j.is_null())
			opt.reset();
		else 
			opt = j.get<T>();
	}
};

template <typename... Types>
struct adl_serializer<std::variant<Types...>> {
	static void to_json(json& j, const std::variant<Types...>& var) {
		std::visit([&](const auto& value) {
			j = json{{"type", var.index()}, {"value", value}};
			}, var);
	}

	static void from_json(const json& j, std::variant<Types...>& var) {
		size_t type_index = j.at("type").get<size_t>();
		const json& value = j.at("value");

		deserialize_variant<0, Types...>(type_index, value, var);
	}

private:
	template <size_t Index, typename T, typename... Rest>
	static void deserialize_variant(size_t type_index, const json& value, std::variant<Types...>& var) {
		if (Index == type_index) {
			var = value.get<T>();
		} else if constexpr (sizeof...(Rest) > 0) {
			deserialize_variant<Index + 1, Rest...>(type_index, value, var);
		} else {
			throw std::runtime_error("Type index out of bounds");
		}
	}
};

}

namespace fmt {
	template <typename T>
	struct formatter<glm::vec<2, T, glm::defaultp>> : formatter<std::string> {
		auto format(glm::vec<2, T, glm::defaultp> my, format_context &ctx) const -> decltype(ctx.out()) {
			return fmt::format_to(ctx.out(), "({},{})", my.x, my.y);
		}
	};

	template <typename T>
	struct formatter<glm::vec<3, T, glm::defaultp>> : formatter<std::string> {
		auto format(glm::vec<3, T, glm::defaultp> my, format_context &ctx) const -> decltype(ctx.out()) {
			return fmt::format_to(ctx.out(), "({},{},{})", my.x, my.y, my.z);
		}
	};

	template <typename T>
	struct formatter<glm::vec<4, T, glm::defaultp>> : formatter<std::string> {
		auto format(glm::vec<4, T, glm::defaultp> my, format_context &ctx) const -> decltype(ctx.out()) {
			return fmt::format_to(ctx.out(), "({},{},{},{})", my.x, my.y, my.z, my.w);
		}
	};
}

namespace NS_DEVKIT {

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

template <unsigned N>
struct string_literal {
	constexpr string_literal(const char(&str)[N]) {
		std::copy_n(str, N, value);
	}
	constexpr string_literal() { };
	char value[N]{};
	const char* c_str() const { return value; }
};

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

template <typename... Ts>
using reverse_tuple = decltype(_reverse_tuple(std::tuple<Ts...>()));

template <typename... Ts> 
struct overload : Ts... { using Ts::operator()...; };

}

namespace NS_DEVKIT {

template <typename T>
class watched {
public:
	watched(T&& value, bool& output) : m_value(value), m_output(output) { }
	watched(T&& value) : m_value(value), m_output(m_outputImpl) { }

	operator const T&() const { return get(); }
	watched& operator= (const T& value) { m_output = true; m_value  = value; return *this; }

	const T& get() const { return m_value; }

private:
	T     m_value;
	bool  m_outputImpl;
	bool& m_output;
};

}

#define CAT_(A, B)                   A ## B
#define CAT(A, B)                    CAT_(A, B)
#define SKIP_IF_EMPTY_CHK(...)       __VA_OPT__(NONEMPTY)
#define SKIP_IF_EMPTY_(...)
#define SKIP_IF_EMPTY_NONEMPTY(...)  __VA_ARGS__
#define SKIP_IF_EMPTY(S, ...)        CAT(SKIP_IF_EMPTY_, SKIP_IF_EMPTY_CHK(S))(__VA_ARGS__)

namespace NS_DEVKIT {

class IFrameProducer {
public:
	virtual bool beginFrame()              = 0;
	virtual void endFrame()	               = 0;
	virtual glm::u32vec2 frameSize() const = 0;
	float aspectRatio() const { return (float)frameSize().x / (float)frameSize().y; }
	virtual void* context()				   = 0;
};

// RAII
class Frame {
public:
	Frame(IFrameProducer& producer)
		: m_producer(producer)
	{ m_beginSuccess = m_producer.beginFrame(); }

	~Frame() { m_producer.endFrame(); }
	operator bool() { return m_beginSuccess; }
private:
	IFrameProducer& m_producer;
	bool		   m_beginSuccess;
};

}

#define _DEVKIT_COLOR_EXPAND_HEX_CHANNELS_RGBA(color, FV) FV(((color >> 24) & 0xFF)), FV(((color >> 16) & 0xFF)), FV(((color >> 8) & 0xFF)), FV((color & 0xFF))
#define _DEVKIT_COLOR_HEX_TO_FLOAT(hex) (float)hex / 255.0
#define _DEVKIT_COLOR_EXPAND_AND_TO_FLOAT(color) _DEVKIT_COLOR_EXPAND_HEX_CHANNELS_RGBA(color, _DEVKIT_COLOR_HEX_TO_FLOAT)

#define DEVKIT_COLOR(hex) glm::vec4{ _DEVKIT_COLOR_EXPAND_AND_TO_FLOAT(hex) }
#define DEVKIT_COLOR_DECL_STATIC(name, hex) inline static constexpr glm::vec4 name = DEVKIT_COLOR(hex);

namespace NS_DEVKIT::colors {

#define _DEVKIT_COLOR_TABLE(FV)  \
	FV( white      , 0xffffffff )\
	FV( black      , 0x000000ff )\
	FV( red        , 0xff0000ff )\
	FV( lime       , 0x00ff00ff )\
	FV( blue       , 0x0000ffff )\
	          					 \
	FV( silver	   , 0xc0c0c0ff )\
	FV( gray	   , 0x808080ff )\
	FV( maroon	   , 0x800000ff )\
	FV( yellow	   , 0xffff00ff )\
	FV( olive	   , 0x808000ff )\
	FV( green      , 0x008000ff )\
	FV( aqua	   , 0x00ffffff )\
	FV( teal	   , 0x008080ff )\
	FV( navy	   , 0x000080ff )\
	FV( fuchsia    , 0xff00ffff )\
	FV( purple	   , 0x800080ff )\
	             				 \
	FV( dodgerBlue , 0x1e90ffff )\
	/* end of table */

_DEVKIT_COLOR_TABLE(DEVKIT_COLOR_DECL_STATIC)
#undef _DEVKIT_COLOR_TABLE

}
