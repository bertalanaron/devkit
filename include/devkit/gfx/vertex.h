#pragma once
#include <devkit/common/utils.h>
#include <devkit/gfx/common.h>

namespace dk::gfx {

class VertexAttributes;

#define DK_VERTEXFLAGS_TABLE(F, ...)                          \
	/*                        Index | Name      | Type     */ \
	F( __VA_ARGS__ __VA_OPT__(,)  0 , Position  , glm::vec3 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  1 , Color0    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  2 , Color1    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  3 , Color2    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  4 , Color3    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  5 , Color4    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  6 , Color5    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  7 , Color6    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  8 , Color7    , glm::vec4 ) \
	F( __VA_ARGS__ __VA_OPT__(,)  9 , Normal    , glm::vec3 ) \
	F( __VA_ARGS__ __VA_OPT__(,) 10 , TexCoords , glm::vec2 ) \
	F( __VA_ARGS__ __VA_OPT__(,) 11 , Tangent   , glm::vec3 ) \
	F( __VA_ARGS__ __VA_OPT__(,) 12 , Bitangent , glm::vec3 ) \
	F( __VA_ARGS__ __VA_OPT__(,) 13 , Bones     , glm::vec4 ) \
	/* end table */

#define DK_DECL_VERTEXFLAGS_ENUM(index, enumName, typeName, ...) \
	enumName = BIT(index),                                       \
	/* end of macro */

enum class VertexFlags : unsigned
{
	DK_VERTEXFLAGS_TABLE(DK_DECL_VERTEXFLAGS_ENUM)
};

constexpr VertexFlags operator|(VertexFlags lhs, VertexFlags rhs)
{ return VertexFlags((unsigned)lhs | (unsigned)rhs); }

template <VertexFlags vf>
struct type_of;

#define DK_DECL_VERTEXFLAGS_TYPE_OF(index, enumName, typeName, ...)               \
	template <> struct type_of<VertexFlags::enumName> { using type = typeName; }; \
	/* end of macro */

DK_VERTEXFLAGS_TABLE(DK_DECL_VERTEXFLAGS_TYPE_OF)

template <VertexFlags vf>
using type_of_t = type_of<vf>::type;

}

namespace dk::gfx {

class VertexAttributes {
private:
	using cache_t = std::unordered_map<std::vector<details::gfx::GLType>, VertexAttributes>;

public:
	static const VertexAttributes* get(std::vector<details::gfx::GLType>&& types);
	static const VertexAttributes* get(VertexFlags flags);

	size_t size() const;

	// @returns The index of the last attribute + 1
	unsigned makePointersActive(size_t indexOffset = 0, unsigned mask = ~0u) const;

	void setPointerDivisors(unsigned divisor = 0, size_t indexOffset = 0, unsigned mask = ~0u) const;

private:
	std::vector<details::gfx::GLType> m_types;
	const size_t                      m_size;

	inline static cache_t s_cache{};

	VertexAttributes(std::vector<details::gfx::GLType>&& types);
};

template <typename... Ts>
class Vertex {
private:
	using data_t = common::reverse_tuple<Ts...>;

	template <std::size_t I>
	using element_t = std::tuple_element_t<sizeof...(Ts) - I - 1, data_t>;

public:
	Vertex() = default;

	Vertex(const Ts&... data)
		: m_data(dk::common::reverse_args(data...))
	{ }

	template <std::size_t I>
	element_t<I>& elem()
	{ return std::get<sizeof...(Ts) - I - 1>(m_data); }

	template <std::size_t I>
	const element_t<I>& elem() const
	{ return std::get<sizeof...(Ts) - I - 1>(m_data); }

	static const VertexAttributes* attributes()
	{ 
		std::vector<details::gfx::GLType> types = { details::gfx::GLType::get<Ts>()... };
		return VertexAttributes::get(std::move(types)); 
	}

	constexpr static std::size_t size() 
	{ return sizeof(data_t); }

protected:
	data_t m_data;
};

template <typename T>
struct is_vertex
	: std::bool_constant<common::is_specialization_or_derived_v<T, Vertex>> 
{};

template <typename T>
constexpr bool is_vertex_v = is_vertex<T>::value;

template <VertexFlags Mask>
struct decl_vertex_from_flags {
	template <VertexFlags Mask, VertexFlags F>
	using maybe_tuple = std::conditional_t<(static_cast<int>(Mask) & static_cast<int>(F)) != 0, std::tuple<type_of_t<F>>, std::tuple<>>;

#define DK_DECL_VERTEXFROMFLAGS_TUPLE(index, enumName, typeName, ...) \
	, maybe_tuple<Mask, VertexFlags::enumName>{}             \
	/* end of macro */

	template <typename Tup>
	struct tuple_to_vertex;

	template <typename... Ts>
	struct tuple_to_vertex<std::tuple<Ts...>> {
		using type = Vertex<Ts...>;
	};

	using tuple_type = decltype(std::tuple_cat(std::tuple<>{}
		DK_VERTEXFLAGS_TABLE(DK_DECL_VERTEXFROMFLAGS_TUPLE)
	));

	using type = typename tuple_to_vertex<tuple_type>::type;
};

template <VertexFlags vf>
using decl_vertex_from_flags_t = typename decl_vertex_from_flags<vf>::type;

using NullVertex = Vertex<>;

struct RGBVertex : public Vertex<glm::vec3, glm::vec3> {
	using Vertex::Vertex;
	auto& postition() { return std::get<0>(m_data); }
	auto& color()     { return std::get<1>(m_data); }
};

struct RGBAVertex : public Vertex<glm::vec3, glm::vec4> {
	using Vertex::Vertex;
	auto& postition() { return std::get<0>(m_data); }
	auto& color()     { return std::get<1>(m_data); }
};

} // dk::gfx
