#pragma once
#include <devkit/common/utils.h>
#include <devkit/gfx/common.h>

namespace dk::gfx {

class VertexAttributes;

enum class VertexFlags {
	Position = BIT(0),
	Color0   = BIT(1),
	Color1   = BIT(2),
	Color2   = BIT(3),
	Color3   = BIT(4),
	Color4   = BIT(5),
	Color5   = BIT(6),
	Color6   = BIT(7),
	Color7   = BIT(8),
	Normals  = BIT(9),
	TexCoord = BIT(10),
	Bones    = BIT(11)
};

VertexFlags operator|(VertexFlags lhs, VertexFlags rhs);

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
	unsigned makePointersActive(size_t indexOffset = 0) const;

	void setPointerDivisors(unsigned divisor = 0, size_t indexOffset = 0) const;

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
